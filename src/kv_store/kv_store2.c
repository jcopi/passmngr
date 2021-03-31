#include <kv_store2.h>

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <dirent.h>

#define ZERO_ARRAY(A)    sodium_memzero(A, sizeof (A))
#define MLOCK_ARRAY(A)   sodium_mlock(A, sizeof (A))
#define MUNLOCK_ARRAY(A) sodium_munlock(A, sizeof(A))

// Private definitions
double current_time ();
bool   has_expired (double previous_time, double max_time);

typedef struct __kv_item_header {
    ITEM_KEY_LENGTH_TYPE key_length;
    ITEM_VALUE_LENGTH_TYPE value_length;
    ITEM_TIMESTAMP_TYPE timestamp;
} __kv_item_header_t;

RESULT_TYPE(__kv_result_header_t, __kv_item_header_t, kv_error_t)

__kv_result_header_t __kv_read_header_from_plaintext_buffer (uint8_t (*buffer)[PLAINTEXT_CHUNK_SIZE], size_t start);
kv_result_t          __kv_write_header_to_plaintext_buffer  (uint8_t (*buffer)[PLAINTEXT_CHUNK_SIZE], size_t start, __kv_item_header_t header);
kv_result_t          __kv_util_get_latest_filename          (kv_t* kv, char (*out_path)[PATH_BUFFER_SIZE]);
kv_result_t          __kv_util_get_next_filename            (kv_t* kv, char (*out_path)[PATH_BUFFER_SIZE]);

// Library functions
kv_result_t kv_initialize (kv_t* kv, const char* const directory)
{
    kv_result_t result = {0};

    // Only uninitialized kvs can be initialized
    assert(kv->state == UNINITIALIZED);

    if (sodium_init() < 0) return ERR(result, KV_ERROR(OUT_OF_MEMORY, KV_ERR_SODIUM_INIT));

    // Zero everything
    ZERO_ARRAY(kv->master_key);
    ZERO_ARRAY(kv->value_buffer);
    ZERO_ARRAY(kv->directory);

    // Fail loudly on filenames that are too long
    size_t directory_length = strlen(directory);
    assert(MAX_DIRECTORY_LENGTH < sizeof (kv->directory));

    // Verify that the directory ends with a seperator
    if (directory[directory_length - 1] == '/' || directory[directory_length - 1] == '\\') {
        assert(directory_length <= MAX_DIRECTORY_LENGTH);
        memcpy(kv->directory, directory, directory_length);
        kv->directory_length = directory_length;
    } else {
        assert(directory_length + 1 <= MAX_DIRECTORY_LENGTH);
        memcpy(kv->directory, directory, directory_length);
        kv->directory[directory_length] = '/';
        kv->directory_length = directory_length + 1;
    }

    // Lock all of the sensitive buffers
    if (MLOCK_ARRAY(kv->master_key) != 0) return ERR(result, KV_ERROR(OUT_OF_MEMORY, KV_ERR_MLOCK));
    if (MLOCK_ARRAY(kv->value_buffer) != 0) return ERR(result, KV_ERROR(OUT_OF_MEMORY, KV_ERR_MLOCK));

    // Initialize variables to constants
    kv->buffers_borrowed = false;
    kv->state = BUFFERS_INITIALIZED;

    return OK_EMPTY(result);
}

kv_result_t kv_create (kv_t* kv, const char* const password, size_t password_length)
{
    kv_result_t result = {0};

    // declare all of the buffers that will be needed to create a kv
    // The master key (plaintext) buffer will be that which is included in the kv struct
    uint8_t master_salt[MASTER_SALT_LENGTH];
    uint8_t password_key[MASTER_KEY_LENGTH];
    uint8_t master_key_ct[MASTER_KEY_CIPHERTEXT_LENGTH];
    uint8_t ciphertext_header[CIPHERTEXT_HEADER_LENGTH];
    char file_path[PATH_BUFFER_SIZE];
    crypto_secretstream_xchacha20poly1305_state st;

    assert(kv->state == BUFFERS_INITIALIZED);

    // Zero all of the buffers
    ZERO_ARRAY(master_salt);
    ZERO_ARRAY(password_key);
    ZERO_ARRAY(master_key_ct);
    ZERO_ARRAY(ciphertext_header);
    ZERO_ARRAY(file_path);
    sodium_memzero(&st, sizeof (st));

    // Error cleanup is important, before leaving this function all of the relevant buffers should be zeroed
    // In order to facilitate this without defer a series of goto points will be created

    // Lock the sensitive (plaintext) buffers
    if (MLOCK_ARRAY(password_key) != 0) {
        SET_ERR(result, KV_ERROR(OUT_OF_MEMORY, KV_ERR_MLOCK));
        goto cleanup;
    }
    if (sodium_mlock(&st, sizeof (st)) != 0) {
        SET_ERR(result, KV_ERROR(OUT_OF_MEMORY, KV_ERR_MLOCK));
        goto cleanup;
    }

    kv_result_t maybe = __kv_util_get_next_filename(&kv->directory, &file_path);
    if (IS_ERR(maybe)) {
        SET_ERR(result, UNWRAP_ERR(maybe));
        goto cleanup;
    }

    // Attempt to open the correct file
    FILE* write_file = fopen(file_path, "wb");
    if (write_file == NULL) {
        // The file could not be opened
        SET_ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
        goto cleanup;
    }

    // Generate a master salt
    randombytes_buf(master_salt, sizeof (master_salt));
    // Write the salt to the front of the file
    size_t bytes_written = fwrite(master_salt, sizeof (uint8_t), sizeof (master_salt), write_file);
    if (bytes_written != sizeof (master_salt)) {
        SET_ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
        goto cleanup_remove_write_file;
    }

    // Use a strong kdf (Argon2ID) to generate a key from the password and salt
    if (crypto_pwhash(password_key, sizeof (password_key), password, password_length, master_salt, 
    crypto_pwhash_argon2id_OPSLIMIT_SENSITIVE, crypto_pwhash_argon2id_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        SET_ERR(result, KV_ERROR(KDF_OPERATION_FAILED, KV_ERR_KDF));
        goto cleanup_remove_write_file;
    }

    // Generate a random master key
    crypto_secretstream_xchacha20poly1305_keygen(kv->master_key);

    // Initialize the secretstream state
    if (crypto_secretstream_xchacha20poly1305_init_push(&st, ciphertext_header, password_key) != 0) {
        SET_ERR(result, KV_ERROR(ENCRYPTION_OPERATION_FAILED, KV_ERR_CRYPTO));
        goto cleanup_remove_write_file;
    }
    // Write the stream header to the file
    bytes_written = fwrite(ciphertext_header, sizeof (uint8_t), sizeof (ciphertext_header),write_file);
    if (bytes_written != sizeof (ciphertext_header)) {
        SET_ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
        goto cleanup_remove_write_file;
    }

    // Encrypt the master key
    if (crypto_secretstream_xchacha20poly1305_push(&st, master_key_ct, NULL, kv->master_key, sizeof (kv->master_key), NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL) != 0) {
        SET_ERR(result, KV_ERROR(ENCRYPTION_OPERATION_FAILED, KV_ERR_CRYPTO));
        goto cleanup_remove_write_file;
    }

    // Write the encrypted master key
    bytes_written = fwrite(master_key_ct, sizeof (uint8_t), sizeof (master_key_ct), write_file);
    if (bytes_written != sizeof (master_key_ct)) {
        SET_ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
        goto cleanup_remove_write_file;
    }

    // flush data to disk
    if (fflush(write_file) != 0) {
        SET_ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
        goto cleanup_remove_write_file;
    }

    fclose(write_file);

    SET_OK_EMPTY(result);
    goto cleanup;

cleanup_remove_write_file:
    fclose(write_file);
    remove(file_path);

cleanup:
    ZERO_ARRAY(master_salt);
    ZERO_ARRAY(password_key);
    ZERO_ARRAY(master_key_ct);
    ZERO_ARRAY(ciphertext_header);
    sodium_memzero(&st, sizeof (st));
    // Creating a kv does not unlock it, so the the master key should be cleared
    ZERO_ARRAY(kv->master_key);

    return result;
}

kv_result_t kv_unlock (kv_t* kv, const char* const password, size_t password_length)
{
    kv_result_t result = {0};

    // declare all of the buffers that will be needed to create a kv
    // The master key (plaintext) buffer will be that which is included in the kv struct
    uint8_t master_salt[MASTER_SALT_LENGTH];
    uint8_t password_key[MASTER_KEY_LENGTH];
    uint8_t master_key_ct[MASTER_KEY_CIPHERTEXT_LENGTH];
    uint8_t ciphertext_header[CIPHERTEXT_HEADER_LENGTH];
    uint8_t plaintext_chunk[PLAINTEXT_CHUNK_SIZE];
    uint8_t ciphertext_chunk[CIPHERTEXT_CHUNK_SIZE];
    char file_path[PATH_BUFFER_SIZE];
    crypto_secretstream_xchacha20poly1305_state st;

    assert(kv->state == BUFFERS_INITIALIZED);

    // Zero all of the buffers
    ZERO_ARRAY(master_salt);
    ZERO_ARRAY(password_key);
    ZERO_ARRAY(master_key_ct);
    ZERO_ARRAY(ciphertext_header);
    ZERO_ARRAY(plaintext_chunk);
    ZERO_ARRAY(ciphertext_chunk);
    ZERO_ARRAY(file_path);
    sodium_memzero(&st, sizeof (st));

    // Error cleanup is important, before leaving this function all of the relevant buffers should be zeroed
    // In order to facilitate this without defer a series of goto points will be created

    // Lock the sensitive (plaintext) buffers
    if (MLOCK_ARRAY(plaintext_chunk) != 0) {
        SET_ERR(result, KV_ERROR(OUT_OF_MEMORY, KV_ERR_MLOCK));
        goto cleanup;
    }
    if (MLOCK_ARRAY(password_key) != 0) {
        SET_ERR(result, KV_ERROR(OUT_OF_MEMORY, KV_ERR_MLOCK));
        goto cleanup;
    }
    if (sodium_mlock(&st, sizeof (st)) != 0) {
        SET_ERR(result, KV_ERROR(OUT_OF_MEMORY, KV_ERR_MLOCK));
        goto cleanup;
    }

    kv_result_t maybe = __kv_util_get_latest_filename(&kv->directory, &file_path);
    if (IS_ERR(maybe)) {
        SET_ERR(result, UNWRAP_ERR(maybe));
        goto cleanup;
    }

    // Attempt to open the correct file
    FILE* read_file = fopen(file_path, "rb");
    if (read_file == NULL) {
        // The file could not be opened
        SET_ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
        goto cleanup;
    }

    // read the salt at the front of the file
    size_t bytes_read = fread(master_salt, sizeof (uint8_t), sizeof (master_salt), read_file);
    if (bytes_read != sizeof (master_salt)) {
        SET_ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
        goto cleanup_close_read_file;
    }

    // Use a strong kdf (Argon2ID) to generate a key from the password and salt
    if (crypto_pwhash(password_key, sizeof (password_key), password, password_length, master_salt, 
    crypto_pwhash_argon2id_OPSLIMIT_SENSITIVE, crypto_pwhash_argon2id_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        SET_ERR(result, KV_ERROR(KDF_OPERATION_FAILED, KV_ERR_KDF));
        goto cleanup_close_read_file;
    }

    bytes_read = fread(ciphertext_header, sizeof (uint8_t), sizeof (ciphertext_header), read_file);
    if (bytes_read != sizeof (ciphertext_header)) {
        SET_ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
        goto cleanup_close_read_file;
    }

    // Initialize the secretstream state
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, ciphertext_header, password_key) != 0) {
        SET_ERR(result, KV_ERROR(ENCRYPTION_OPERATION_FAILED, KV_ERR_CRYPTO));
        goto cleanup_close_read_file;
    }

    // Read the encrypted master key
    bytes_read = fread(master_key_ct, sizeof (uint8_t), sizeof (master_key_ct), read_file);
    if (bytes_read != sizeof (ciphertext_chunk)) {
        SET_ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
        goto cleanup_close_read_file;
    }

    // Decrypt the master key
    unsigned char tag;
    if (crypto_secretstream_xchacha20poly1305_pull(&st, kv->master_key, NULL, master_key_ct, sizeof (master_key_ct), NULL, 0, &tag) != 0) {
        SET_ERR(result, KV_ERROR(ENCRYPTION_OPERATION_FAILED, KV_ERR_CRYPTO));
        goto cleanup_close_read_file;
    }

    fclose(read_file);
    kv->state = UNLOCKED;

    SET_OK_EMPTY(result);
    goto cleanup;

cleanup_close_read_file:
    fclose(read_file);
cleanup:
    ZERO_ARRAY(master_salt);
    ZERO_ARRAY(password_key);
    ZERO_ARRAY(master_key_ct);
    ZERO_ARRAY(ciphertext_header);
    ZERO_ARRAY(plaintext_chunk);
    ZERO_ARRAY(ciphertext_chunk);
    sodium_memzero(&st, sizeof (st));

    return result;
}

kv_result_opt_value_t kv_get (kv_t* kv, kv_key_t key)
{
    kv_result_opt_value_t result = {0};
    kv_opt_value_t value = {0};

    uint8_t ciphertext_header[CIPHERTEXT_HEADER_LENGTH];
    uint8_t ciphertext_chunk[CIPHERTEXT_CHUNK_SIZE];
    uint8_t plaintext_chunk[PLAINTEXT_CHUNK_SIZE];
    char file_path[PATH_BUFFER_SIZE]
}

kv_result_t kv_set (kv_t* kv, kv_key_t key, kv_value_t value);
kv_result_t kv_delete (kv_t* kv, kv_key_t key);

kv_result_opt_value_t kv_borrow_value (kv_t* kv);
void kv_release_value (kv_t* kv);

__kv_result_header_t __kv_read_header_from_plaintext_buffer (uint8_t (*buffer)[PLAINTEXT_CHUNK_SIZE], size_t start)
{
    __kv_result_header_t result = {0};
    __kv_item_header_t header = {0};


}

kv_result_t __kv_write_header_to_plaintext_buffer (uint8_t (*buffer)[PLAINTEXT_CHUNK_SIZE], size_t start, __kv_item_header_t header)
{
    kv_result_t result = {0};
}

kv_result_t __kv_util_get_latest_filename (kv_t* kv, char (*out_path)[PATH_BUFFER_SIZE])
{
    assert(kv->state == BUFFERS_INITIALIZED);

    kv_result_t result = {0};

    struct dirent* dp;
    DIR* dir = opendir(kv->directory);
    if (dir == NULL) {
        return ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
    }

    DEFAULT_FILE_COUNTER_TYPE max_counter = 0;
    char file_name[DEFAULT_FILENAME_LENGTH] = {0};

    while ((dp = readdir(dir)) != NULL) {
        if (dp->d_type == DT_REG) {
            enum {PREFIX, COUNTER, EXTENSION, COMPLETE} state = PREFIX;
            bool valid_name = false;
            uint64_t tmp_counter = 0;

            size_t d_name_length = strlen(dp->d_name);
            if (d_name_length != sizeof (file_name)) {
                continue;
            }

            for (size_t i = 0, j = 0; dp->d_name[i] != '\0'; i++) {
                if (state == PREFIX) {
                    if (dp->d_name[i] != DEFAULT_FILENAME_PREFIX[j]) {
                        break;
                    } else {
                        j++;
                        if (j >= FILENAME_PREFIX_LENGTH) {
                            j = 0;
                            state = COUNTER;
                        }
                    }
                } else if (state == COUNTER) {
                    if (dp->d_name[i] >= '0' && dp->d_name[i] <= '9') {
                        tmp_counter *= 16;
                        tmp_counter += dp->d_name[i] - '0';
                        j++;
                        if (j >= DEFAULT_FILE_COUNTER_LENGTH) {
                            j = 0;
                            state = EXTENSION;
                        }
                    } else if (dp->d_name[i] >= 'a' && dp->d_name[i] <= 'f') {
                        tmp_counter *= 16;
                        tmp_counter += 10;
                        tmp_counter += dp->d_name[i] - 'a';
                        j++;
                        if (j >= DEFAULT_FILE_COUNTER_LENGTH) {
                            j = 0;
                            state = EXTENSION;
                        }
                    } else if (dp->d_name[i] >= 'A' && dp->d_name[i] <= 'F') {
                        tmp_counter *= 16;
                        tmp_counter += 10;
                        tmp_counter += dp->d_name[i] - 'A';
                        j++;
                        if (j >= DEFAULT_FILE_COUNTER_LENGTH) {
                            j = 0;
                            state = EXTENSION;
                        }
                    } else {
                        break;
                    }
                } else if (state == EXTENSION) {
                    if (dp->d_name[i] != DEFAULT_FILE_EXTENSION[j]) {
                        break;
                    } else {
                        j++;
                        if (j >= FILE_EXTENSION_LENGTH) {
                            j = 0;
                            state = COMPLETE;
                            valid_name = true;
                        }
                    }
                } else if (state == COMPLETE) {
                    valid_name = false;
                    break;
                }
            }

            if (!valid_name) continue;

            if (tmp_counter > max_counter) {
                max_counter = tmp_counter;
                memcpy(file_name, dp->d_name, sizeof(file_name));
            }
        }
    }

    closedir(dir);

    ZERO_ARRAY(*out_path);

    size_t i = 0;
    memcpy(&(*out_path)[i], kv->directory, kv->directory_length);
    i += kv->directory_length;
    memcpy(&(*out_path)[i], file_name, sizeof (file_name));
    i += sizeof (file_name);
    (*out_path)[i] = '\0';

    return OK_EMPTY(result);
}

kv_result_t __kv_util_get_next_filename (kv_t* kv, char (*out_path)[PATH_BUFFER_SIZE])
{
    assert(kv->state == BUFFERS_INITIALIZED);

    kv_result_t result = {0};

    struct dirent* dp;
    DIR* dir = opendir(kv->directory);
    if (dir == NULL) {
        return ERR(result, KV_ERROR(FILE_OPERATION_FAILED, strerror(errno)));
    }

    DEFAULT_FILE_COUNTER_TYPE max_counter = 0;
    char file_name[DEFAULT_FILENAME_LENGTH] = {0};

    while ((dp = readdir(dir)) != NULL) {
        if (dp->d_type == DT_REG) {
            enum {PREFIX, COUNTER, EXTENSION, COMPLETE} state = PREFIX;
            bool valid_name = false;
            uint64_t tmp_counter = 0;
            for (size_t i = 0, j = 0; dp->d_name[i] != '\0'; i++) {
                if (state == PREFIX) {
                    if (dp->d_name[i] != DEFAULT_FILENAME_PREFIX[j]) {
                        break;
                    } else {
                        j++;
                        if (j >= FILENAME_PREFIX_LENGTH) {
                            j = 0;
                            state = COUNTER;
                        }
                    }
                } else if (state == COUNTER) {
                    if (dp->d_name[i] >= '0' && dp->d_name[i] <= '9') {
                        tmp_counter *= 16;
                        tmp_counter += dp->d_name[i] - '0';
                        j++;
                        if (j >= DEFAULT_FILE_COUNTER_LENGTH) {
                            j = 0;
                            state = EXTENSION;
                        }
                    } else if (dp->d_name[i] >= 'a' && dp->d_name[i] <= 'f') {
                        tmp_counter *= 16;
                        tmp_counter += 10;
                        tmp_counter += dp->d_name[i] - 'a';
                        j++;
                        if (j >= DEFAULT_FILE_COUNTER_LENGTH) {
                            j = 0;
                            state = EXTENSION;
                        }
                    } else if (dp->d_name[i] >= 'A' && dp->d_name[i] <= 'F') {
                        tmp_counter *= 16;
                        tmp_counter += 10;
                        tmp_counter += dp->d_name[i] - 'A';
                        j++;
                        if (j >= DEFAULT_FILE_COUNTER_LENGTH) {
                            j = 0;
                            state = EXTENSION;
                        }
                    } else {
                        break;
                    }
                } else if (state == EXTENSION) {
                    if (dp->d_name[i] != DEFAULT_FILE_EXTENSION[j]) {
                        break;
                    } else {
                        j++;
                        if (j >= FILE_EXTENSION_LENGTH) {
                            j = 0;
                            state = COMPLETE;
                            valid_name = true;
                        }
                    }
                } else if (state == COMPLETE) {
                    valid_name = false;
                    break;
                }
            }

            if (!valid_name) continue;

            if (tmp_counter > max_counter) {
                max_counter = tmp_counter;
            }
        }
    }

    closedir(dir);

    ZERO_ARRAY(*out_path);
    max_counter += 1;

    size_t i = 0;
    memcpy(&(*out_path)[i], kv->directory, kv->directory_length);
    i += kv->directory_length;
    memcpy(&(*out_path)[i], DEFAULT_FILENAME_PREFIX, FILENAME_PREFIX_LENGTH);
    i += FILENAME_PREFIX_LENGTH;
    // Copy number into file path
    for (size_t j = 1; j <= DEFAULT_FILE_COUNTER_LENGTH; j++) {
        char hex_digit = 0;
        uint64_t masked = max_counter & (0xf << (DEFAULT_FILE_COUNTER_LENGTH - j));
        hex_digit = masked >> (DEFAULT_FILE_COUNTER_LENGTH - j);

        if (hex_digit < 0xa) {
            (*out_path)[i + (j - 1)] = '0' + hex_digit;
        } else if (hex_digit < 0x10) {
            (*out_path)[i + (j - 1)] = 'a' + hex_digit;
        } else {
            // This definitely can't happen
            assert(false);
        }
    }
    i += DEFAULT_FILE_COUNTER_LENGTH;
    memcpy(&(*out_path)[i], DEFAULT_FILE_EXTENSION, FILE_EXTENSION_LENGTH);
    i += FILE_EXTENSION_LENGTH;
    (*out_path)[i] = '\0';

    return OK_EMPTY(result);
}
