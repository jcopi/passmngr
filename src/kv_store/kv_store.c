#include <kv_store.h>

#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#if true //defined(_POSIX_MONOTONIC_CLOCK)

    // #define __USE_POSIX199309
    #include <time.h>

    double kv_util_get_monotonic_time ()
    {
        /*#if defined(CLOCK_MONOTONIC_RAW)
        #define CLOCK_ID CLOCK_MONOTONIC_RAW
        #elif defined(CLOCK_MONOTONIC)
        #define CLOCK_ID CLOCK_MONOTONIC
        #elif defined(CLOCK_BOOTTIME)
        #define CLOCK_ID CLOCK_BOOTTIME
        #endif*/

        struct timespec ts;
        if (timespec_get(&ts, TIME_UTC) == 0) {
            return 0.0;
        }
        //clock_gettime(CLOCK_ID, &ts);
        return ((double) ts.tv_sec) + ((double) ts.tv_nsec / 1e-9);
    }

#elif defined(__APPLE__)

    #include <CoreServices/CoreServices.h>
    #include <mach/mach_time.h>

    double kv_util_get_monotonic_time ()
    {
        uint64_t time = mach_absolute_time();
        Nanoseconds ns = AbsoluteToNanosecons(*(AbsoluteTime*) &time);
        return (*(uint64_t*) &ns) * 1e-9;
    }

#elif defined(_MSC_VER)

    #include <windows.h>
    
    double kv_util_get_monotonic_time ()
    {
        return GetTickCount64() * 1e-3;
    }

#endif

void print_hex_memory (char* prefix, void* mem, size_t length)
{
    printf("%s (%i B):\n", prefix, (int) length);
    for (size_t i = 0; i < length; i++) {
        printf("%02x ", ((unsigned char*)mem)[i]);
        if (i && !(i % 16)) {
            printf("\n");
        }
    }
    printf("\n");
}

bool kv_util_is_past_expiration(double start_time, double max_time)
{
    double current_time = kv_util_get_monotonic_time();
    // attempt to check if clock has overflowed or if the elapsed time exceeds max
    return (current_time > start_time) && ((current_time - start_time) < max_time);
}

bool __kv_file_exists (const char* filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

kv_empty_result_t kv_set_default_settings (kv_t* kv)
{
    kv_empty_result_t result = {0};
    return OK_EMPTY(result);
}

kv_header_result_t __kv_parse_item_header (byte_t (*buffer)[PLAINTEXT_CHUNK_SIZE], size_t start)
{
    kv_header_result_t result = {0};
    kv_item_header_t header = {0};

    if (start + sizeof (header.key_length) > sizeof (*buffer)) {
        return ERR(result, FATAL_MALFORMED_FILE);
    }
    memcpy(&header.key_length, &(*buffer)[start], sizeof (header.key_length));
    start += sizeof (header.key_length);

    if (start + sizeof (header.value_length) > sizeof (*buffer)) {
        return ERR(result, FATAL_MALFORMED_FILE);
    }
    memcpy(&header.value_length, &(*buffer)[start], sizeof (header.value_length));
    start += sizeof (header.value_length);

    if (header.value_length <= 0 || header.key_length <= 0 || header.key_length + header.value_length > ITEM_MAX_CONTENT_SIZE) {
        return ERR(result, FATAL_MALFORMED_FILE);
    }

    if (start + sizeof (header.timestamp) > sizeof (*buffer)) {
        return ERR(result, FATAL_MALFORMED_FILE);
    }
    memcpy(&header.timestamp, &(*buffer)[start], sizeof (header.timestamp));
    start += sizeof (header.timestamp);

    if (header.key_length + header.value_length + start > sizeof (*buffer)) {
        return ERR(result, FATAL_MALFORMED_FILE);
    }

    return OK(result, header);
}

kv_empty_result_t __kv_write_item (byte_t (*buffer)[PLAINTEXT_CHUNK_SIZE], size_t start, kv_item_t* item)
{
    kv_empty_result_t result = {0};

    size_t total_item_length = ITEM_HEADER_SIZE + item->key.length + item->value.length;
    if (start + total_item_length > sizeof (*buffer)) {
        return ERR(result, RUNTIME_ITEM_TOO_LARGE);
    }

    memcpy(&(*buffer)[start], &item->key.length, sizeof (item->key.length));
    start += sizeof (item->key.length);
    memcpy(&(*buffer)[start], &item->value.length, sizeof (item->value.length));
    start += sizeof (item->value.length);
    // Get current time to use as timestamp
    struct timespec ts = {0};
    if (timespec_get(&ts, TIME_UTC) == 0) {
        // Error occurred getting time
        return ERR(result, RUNTIME_CLOCK_ERROR);
    }
    ITEM_TIMESTAMP_INTEGER_TYPE timestamp = (ITEM_TIMESTAMP_INTEGER_TYPE) ts.tv_sec;
    memcpy(&(*buffer)[start], &timestamp, sizeof (timestamp));
    start += sizeof (timestamp);

    memcpy(&(*buffer)[start], item->key.data, item->key.length);
    start += item->key.length;

    memcpy(&(*buffer)[start], item->value.data, item->value.length);
    start += item->value.length;

    return OK_EMPTY(result);
}

kv_empty_result_t kv_init(kv_t* kv, const char* const read_filename, const char* const write_filename)
{
    kv_empty_result_t result = {0};

    if (sodium_init() < 0) {
        return ERR(result, FATAL_OUT_OF_MEMORY);
    }

    if (sodium_mlock(kv->master_key, sizeof (kv->master_key)) != 0) {
        return ERR(result, FATAL_OUT_OF_MEMORY);
    }

    sodium_memzero(kv->master_key, sizeof (kv->master_key));

    kv->tmps_borrowed = false;

    kv->read_filename = read_filename;
    kv->write_filename = write_filename;

    kv->initialized = true;

    return OK_EMPTY(result);
}

kv_empty_result_t kv_create (kv_t* kv, const char * const password, size_t password_length)
{
    assert(kv->initialized);
    
    // This function will open a kv for reading or create a new kv then open it if the provided filename does not exist
    // Check that the provided file name exists
    kv_empty_result_t result = {0};

    byte_t master_salt[crypto_pwhash_argon2id_SALTBYTES];
    byte_t password_key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    byte_t encrypted_key_ciphertext[crypto_secretstream_xchacha20poly1305_KEYBYTES + crypto_secretstream_xchacha20poly1305_ABYTES];
    byte_t encrypted_key_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;

    // if the provided kv is already open do nothing
    // currently this will not be treated as an error, but that may change in the future
    if (kv->unlocked) {
        SET_OK_EMPTY(result);
        goto exit_success;
    }

    // Sensitive buffers should be locked to prevent swapping to disk and being printed in kernel logs
    // ciphertexts and salts aren't secret so the don't strictly need to be locked
    if (sodium_mlock(password_key, sizeof (password_key)) != 0 || sodium_mlock(&st, sizeof (st)) != 0) {
        SET_ERR(result, FATAL_OUT_OF_MEMORY);
        goto exit_error;
    }

    kv->max_unlock_time_s = DEFAULT_MAX_UNLOCK_TIME;
    
    // open the file for writing, normally writing would only happen on the `write_filename`
    // in this case since the read file name doesn't exist theres no need to do the write, delete, rename sequence
    kv->file = fopen(kv->read_filename, "wb");
    if (kv->file == NULL) {
        SET_ERR(result, FILE_OPEN_FAILED);
        goto exit_error;
    }

    // generate and write a master salt
    randombytes_buf(master_salt, sizeof (master_salt));
    size_t nwritten = fwrite(master_salt, sizeof (byte_t), sizeof (master_salt), kv->file);
    if (nwritten != sizeof (master_salt)) {
        SET_ERR(result, FILE_WRITE_FAILED);
        goto exit_error;
    }

    // generate the password key
    if (crypto_pwhash(password_key, sizeof (password_key), password, password_length, master_salt, 
    crypto_pwhash_argon2id_OPSLIMIT_SENSITIVE, crypto_pwhash_argon2id_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        SET_ERR(result, FATAL_OUT_OF_MEMORY);
        goto exit_error;
    }

    // Generate a master key and write it to a file
    crypto_secretstream_xchacha20poly1305_keygen(kv->master_key);
    
    unsigned char tag;
    if (crypto_secretstream_xchacha20poly1305_init_push(&st, encrypted_key_header, password_key) != 0) {
        SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
        goto exit_error;
    }

    nwritten = fwrite(encrypted_key_header, sizeof (byte_t), sizeof (encrypted_key_header), kv->file);
    if (nwritten != sizeof (encrypted_key_header)) {
        SET_ERR(result, FILE_WRITE_FAILED);
        goto exit_error;
    }
    
    tag = crypto_secretstream_xchacha20poly1305_TAG_FINAL;
    if (crypto_secretstream_xchacha20poly1305_push(&st, encrypted_key_ciphertext, NULL, kv->master_key, sizeof (kv->master_key), NULL, 0, tag) != 0) {
        SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
        goto exit_error;
    }

    nwritten = fwrite(encrypted_key_ciphertext, sizeof (byte_t), sizeof (encrypted_key_ciphertext), kv->file);
    if (nwritten != sizeof (encrypted_key_ciphertext)) {
        SET_ERR(result, FILE_WRITE_FAILED);
        goto exit_error;
    }

    kv->kv_start = ftell(kv->file);
    kv->kv_empty = true;

    // close the file
    fclose(kv->file);

    SET_OK_EMPTY(result);
    goto exit_success;

exit_success:
    goto cleanup;
exit_error:
    // For opening in all cases that there is an error the master key should be cleared
    sodium_memzero(kv->master_key, sizeof (kv->master_key));
    goto cleanup;
cleanup:
    // zero all buffers before leaving the function
    sodium_memzero(master_salt, sizeof (master_salt));
    sodium_memzero(password_key, sizeof (password_key));
    sodium_memzero(encrypted_key_ciphertext, sizeof (encrypted_key_ciphertext));
    sodium_memzero(encrypted_key_header, sizeof (encrypted_key_header));
    sodium_memzero(&st, sizeof (st));

    return result;
}

kv_empty_result_t kv_open (kv_t* kv, const char * const password, size_t password_length)
{
    assert(kv->initialized);
    
    // This function will open a kv for reading or create a new kv then open it if the provided filename does not exist
    // Check that the provided file name exists
    kv_empty_result_t result = {0};

    byte_t master_salt[crypto_pwhash_argon2id_SALTBYTES];
    byte_t password_key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    byte_t encrypted_key_ciphertext[crypto_secretstream_xchacha20poly1305_KEYBYTES + crypto_secretstream_xchacha20poly1305_ABYTES];
    byte_t encrypted_key_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;

    // if the provided kv is already open do nothing
    // currently this will not be treated as an error, but that may change in the future
    if (kv->unlocked) {
        SET_OK_EMPTY(result);
        goto exit_success;
    }

    // Sensitive buffers should be locked to prevent swapping to disk and being printed in kernel logs
    // ciphertexts and salts aren't secret so the don't strictly need to be locked
    if (sodium_mlock(password_key, sizeof (password_key)) != 0 || sodium_mlock(&st, sizeof (st)) != 0) {
        SET_ERR(result, FATAL_OUT_OF_MEMORY);
        goto exit_error;
    }

    kv->max_unlock_time_s = DEFAULT_MAX_UNLOCK_TIME;

    // File exists, open it and begin reading the header
    kv->file = fopen(kv->read_filename, "rb");
    if (kv->file == NULL) {
        SET_ERR(result, FILE_OPEN_FAILED);
        goto exit_error;
    }

    // read the master salt
    size_t nread = fread(master_salt, sizeof (byte_t), sizeof (master_salt), kv->file);
    if (nread != sizeof (master_salt)) {
        SET_ERR(result, FILE_READ_FAILED);
        goto exit_error;
    }

    // Generate the password key
    if (crypto_pwhash(password_key, sizeof (password_key), password, password_length, master_salt, 
    crypto_pwhash_argon2id_OPSLIMIT_SENSITIVE, crypto_pwhash_argon2id_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        SET_ERR(result, FATAL_OUT_OF_MEMORY);
        goto exit_error;
    }

    // decrypt the master key and read it
    unsigned char tag;
    nread = fread(encrypted_key_header, sizeof (byte_t), sizeof (encrypted_key_header), kv->file);
    if (nread != sizeof (encrypted_key_header)) {
        SET_ERR(result, FILE_READ_FAILED);
        goto exit_error;
    }

    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, encrypted_key_header, password_key) != 0) {
        SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
        goto exit_error;
    }

    nread = fread(encrypted_key_ciphertext, sizeof (byte_t), sizeof (encrypted_key_ciphertext), kv->file);
    if (nread != sizeof (encrypted_key_ciphertext)) {
        SET_ERR(result, FILE_READ_FAILED);
        goto exit_error;
    }

    if (crypto_secretstream_xchacha20poly1305_pull(&st, kv->master_key, NULL, &tag,
    encrypted_key_ciphertext, sizeof (encrypted_key_ciphertext), NULL, 0) != 0) {
        SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
        goto exit_error;
    }

    kv->unlock_start_time_s = kv_util_get_monotonic_time();
    kv->unlocked = true;
    kv->kv_start = ftell(kv->file);

    SET_OK_EMPTY(result);

    goto exit_success;

exit_success:
    goto cleanup;
exit_error:
    // For opening in all cases that there is an error the master key should be cleared
    sodium_memzero(kv->master_key, sizeof (kv->master_key));
    goto cleanup;
cleanup:
    // zero all buffers before leaving the function
    sodium_memzero(master_salt, sizeof (master_salt));
    sodium_memzero(password_key, sizeof (password_key));
    sodium_memzero(encrypted_key_ciphertext, sizeof (encrypted_key_ciphertext));
    sodium_memzero(encrypted_key_header, sizeof (encrypted_key_header));
    sodium_memzero(&st, sizeof (st));

    return result;
}

kv_value_result_t kv_get (kv_t* kv, kv_key_t key)
{
    assert(kv->initialized);
    assert(!kv->tmps_borrowed);

    //kv_value_t value = {0};
    kv_value_result_t result = {0};

    byte_t ct_read_buffer[CIPHERTEXT_CHUNK_SIZE];
    byte_t ct_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    byte_t read_buffer[PLAINTEXT_CHUNK_SIZE];
    crypto_secretstream_xchacha20poly1305_state st;

    size_t bytes_read;

    if (!kv->unlocked) {
        SET_ERR(result, RUNTIME_NOT_OPEN);
        goto exit_error;
    }

    if (key.length > ITEM_MAX_CONTENT_SIZE) {
        // invalid key
        SET_ERR(result, RUNTIME_ITEM_TOO_LARGE);
        goto exit_error;
    }

    // cipher text buffer don't strictly need to be locked
    if (sodium_mlock(read_buffer, sizeof (read_buffer)) != 0 || sodium_mlock(&st, sizeof (st)) != 0 || sodium_mlock(result.result.value.data, sizeof (result.result.value.data)) != 0) {
        SET_ERR(result, FATAL_OUT_OF_MEMORY);
        goto exit_error;
    }

    // Attempt to seek to the start of the kv
    if (fseek(kv->file, kv->kv_start, SEEK_SET) != 0) {
        SET_ERR(result, FILE_SEEK_FAILED);
        goto exit_error;
    }

    // If at the end of the file then no entries exist, exit
    if (feof(kv->file)) {
        SET_ERR(result, RUNTIME_ITEM_NOT_FOUND);
        goto exit_error;
    }

    // If there are any entries in the kv store it should have at least HEADERBYTES + ABYTES to read
    // Read the secretstream header
    bytes_read = fread(ct_header, sizeof (byte_t), sizeof (ct_header), kv->file);
    if (bytes_read != sizeof (ct_header)) {
        SET_ERR(result, FILE_READ_FAILED);
        goto exit_error;
    }

    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, ct_header, kv->master_key) != 0) {
        SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
        goto exit_error;
    }
    
    // If there is not data after the header this will be treated as an error.
    // If the kv is empty the header should not be populated
    if (feof(kv->file)) {
        SET_ERR(result, FATAL_MALFORMED_FILE);
        goto exit_error;
    }

    unsigned char tag;
    do {
        bytes_read = fread(ct_read_buffer, sizeof (byte_t), sizeof (ct_read_buffer), kv->file);
        if (bytes_read != sizeof (ct_read_buffer)) {
            // Data will always be padded (with 0s) to a multiple of the default chunk size
            // That makes this case an error and eliminates the need to handle this particular edge
            SET_ERR(result, FATAL_MALFORMED_FILE);
            goto exit_error;
        }
        // In addition to padding the end of the kv to a multiple of chunk size, the kv should be padded such that no entry overlaps a chunk.
        // In practice this means that the total length of the key and value cannot exceed CHUNK SIZE - 24.
        // This is a substantial limitation, but in the typical usage it should not be particularly detrimental. 
        if (crypto_secretstream_xchacha20poly1305_pull(&st, read_buffer, NULL, &tag, ct_read_buffer, sizeof (ct_read_buffer), NULL, 0) != 0) {
            SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
            goto exit_error;
        }

        size_t i = 0;

        while (i < sizeof (read_buffer)) {
            size_t remaining = sizeof (read_buffer) - i;
            if (remaining <= ITEM_HEADER_SIZE || sodium_is_zero(&read_buffer[i], remaining)) {
                // An entry could not fit here or all remaining bytes are zero, so the rest must be padded.
                break;
            }

            kv_header_result_t maybe_header = __kv_parse_item_header(&read_buffer, i);
            if (IS_ERR(maybe_header)) {
                SET_ERR(result, UNWRAP_ERR(maybe_header));
                goto exit_error;
            }

            kv_item_header_t header = UNWRAP(maybe_header);
            i += ITEM_HEADER_SIZE;

            if (header.key_length == key.length && sodium_memcmp(&read_buffer[i], key.data, header.key_length) == 0) {
                // This is the matching key
                // This is a non standard usage of the result "library"
                i += header.key_length;
                memcpy(result.result.value.data, &read_buffer[i], header.value_length);
                result.result.value.length = header.value_length;
                SET_OK_EMPTY(result);

                goto exit_success;
            } else {
                i += header.key_length + header.value_length;
            }
        }
    } while (!feof(kv->file) && tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL);

    SET_ERR(result, RUNTIME_ITEM_NOT_FOUND);
    goto exit_error;

exit_success:
    goto cleanup;
exit_error:
    goto cleanup;
cleanup:
    sodium_memzero(ct_header, sizeof (ct_header));
    sodium_memzero(ct_read_buffer, sizeof (ct_read_buffer));
    sodium_memzero(read_buffer, sizeof (read_buffer));
    sodium_memzero(&st, sizeof (st));

    return result;
}

kv_empty_result_t kv_set (kv_t* kv, kv_item_t item)
{
    // set works similarly to get, but values can't be inserted into the middle of an encrypted kv
    // the initial header (salt & encrypted master) need to be copied over as is to the `write_filename`
    // the data in the kv needs to be read, decrypted, modified (if applicable), re-encrypted, then written to the new file

    assert(kv->initialized);
    assert(!kv->tmps_borrowed);

    kv_empty_result_t result = {0};

    byte_t ct_buffer[CIPHERTEXT_CHUNK_SIZE];
    byte_t ct_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    byte_t buffer[PLAINTEXT_CHUNK_SIZE];
    crypto_secretstream_xchacha20poly1305_state st_in;
    crypto_secretstream_xchacha20poly1305_state st_out;

    sodium_memzero(ct_buffer, sizeof (ct_buffer));
    sodium_memzero(ct_header, sizeof (ct_header));
    sodium_memzero(buffer, sizeof (buffer));
    sodium_memzero(&st_in, sizeof (st_in));
    sodium_memzero(&st_out, sizeof (st_out));

    size_t bytes_read;
    size_t bytes_written;

    if (!kv->unlocked) {
        SET_ERR(result, RUNTIME_NOT_OPEN);
        goto exit_error;
    }

    // Don't attempt anything if the provided key and values woun't fit into a single chunk
    // The total entry length will be useful later so it will be stored
    uint64_t total_entry_length = ITEM_HEADER_SIZE;
    total_entry_length += item.key.length;
    total_entry_length += item.value.length;

    if (total_entry_length > PLAINTEXT_CHUNK_SIZE || item.key.length <= 0 || item.value.length <= 0) {
        // This key/value pair won't fit in the kv store
        SET_ERR(result, RUNTIME_ITEM_TOO_LARGE);
        goto exit_error;
    }

    // cipher text buffer don't strictly need to be locked
    if (sodium_mlock(buffer, sizeof (buffer)) != 0 || sodium_mlock(&st_in, sizeof (st_in)) != 0 ||
        sodium_mlock(&st_out, sizeof (st_out)) != 0) {
        SET_ERR(result, FATAL_OUT_OF_MEMORY);
        goto exit_error;
    }

    // Attempt to seek to the start of the read file
    if (fseek(kv->file, 0, SEEK_SET) != 0) {
        SET_ERR(result, FILE_SEEK_FAILED);
        goto exit_error;
    }

    // The first step is to create a new file and copy the master salt and encrypted password.
    FILE* write_file = fopen(kv->write_filename, "wb");
    if (write_file == NULL) {
        SET_ERR(result, FILE_OPEN_FAILED);
        goto exit_error;
    }

    // Read the entire starting header from the read file and write it to the write file
    byte_t header_buffer[crypto_pwhash_argon2id_SALTBYTES + crypto_secretstream_xchacha20poly1305_HEADERBYTES + crypto_secretstream_xchacha20poly1305_KEYBYTES + crypto_secretstream_xchacha20poly1305_ABYTES];
    bytes_read = fread(header_buffer, sizeof (byte_t), sizeof (header_buffer), kv->file);
    if (bytes_read != sizeof (header_buffer)) {
        SET_ERR(result, FILE_READ_FAILED);
        goto exit_error;
    }
    bytes_written = fwrite(header_buffer, sizeof (byte_t), sizeof (header_buffer), write_file);
    if (bytes_written != sizeof (header_buffer)) {
        SET_ERR(result, FILE_WRITE_FAILED);
        goto exit_error;
    }

    if (crypto_secretstream_xchacha20poly1305_init_push(&st_out, ct_header, kv->master_key) != 0) {
        SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
        goto exit_error;
    }
    bytes_written = fwrite(ct_header, sizeof (byte_t), sizeof (ct_header), write_file);
    if (bytes_written != sizeof (ct_header)) {
        SET_ERR(result, FILE_WRITE_FAILED);
        goto exit_error;
    }

    sodium_memzero(ct_header, sizeof (ct_header));

    bytes_read = fread(ct_header, sizeof (byte_t), sizeof (ct_header), kv->file);
    // No items exist yet
    if (bytes_read != sizeof (ct_header) && feof(kv->file)) {
        sodium_memzero(buffer, sizeof (buffer));
        sodium_memzero(ct_buffer, sizeof (ct_buffer));
        kv_empty_result_t maybe_result = __kv_write_item(&buffer, 0, &item);
        if (IS_ERR(maybe_result)) {
            SET_ERR(result, UNWRAP_ERR(maybe_result));
            goto exit_error;
        }
        if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, buffer, sizeof (buffer), NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL) != 0) {
            SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
            goto exit_error;
        }
        bytes_written = fwrite(ct_buffer, sizeof (byte_t), sizeof (ct_buffer), write_file);
        if (bytes_written != sizeof (ct_buffer)) {
            SET_ERR(result, FILE_WRITE_FAILED);
            goto exit_error;
        }
    } else if (bytes_read == sizeof (ct_header)) {
        // Read and write the secretstream headers
        
        if (crypto_secretstream_xchacha20poly1305_init_pull(&st_in, ct_header, kv->master_key) != 0) {
            SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
            goto exit_error;
        }

        unsigned char read_tag;
        bool item_written = false;

        do { // This is the chunk level loop
            sodium_memzero(buffer, sizeof (buffer));
            sodium_memzero(ct_buffer, sizeof (ct_buffer));

            bytes_read = fread(ct_buffer, sizeof (byte_t), sizeof (ct_buffer), kv->file);
            if (bytes_read != sizeof (ct_buffer)) {
                // Data will always be padded (with 0s) to a multiple of the default chunk size
                // That makes this case an error and eliminates the need to handle this particular edge
                SET_ERR(result, FATAL_MALFORMED_FILE);
                goto exit_error;
            }
            // In addition to padding the end of the kv to a multiple of chunk size, the kv should be padded such that no entry overlaps a chunk.
            // In practice this means that the total length of the key and value cannot exceed CHUNK SIZE - 24.
            // This is a substantial limitation, but in the typical usage it should not be particularly detrimental. 
            if (crypto_secretstream_xchacha20poly1305_pull(&st_in, buffer, NULL, &read_tag, ct_buffer, sizeof (ct_buffer), NULL, 0) != 0) {
                SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
                goto exit_error;
            }

            size_t i = 0;
            size_t remaining = 0;

            while (i < sizeof (buffer)) {
                remaining = sizeof (buffer) - i;
                if (remaining <= ITEM_HEADER_SIZE || sodium_is_zero(&buffer[i], remaining)) {
                    // An entry could not fit here or all remaining bytes are zero, so the rest must be padded.
                    break;
                }

                // Copy i into j so that increments can be rolled back
                kv_header_result_t maybe_header = __kv_parse_item_header(&buffer, i);
                if (IS_ERR(maybe_header)) {
                    SET_ERR(result, UNWRAP_ERR(maybe_header));
                    print_hex_memory("buffer", buffer, sizeof (buffer));
                    sodium_memzero(buffer, sizeof (buffer));
                    print_hex_memory("buffer", buffer, sizeof (buffer));
                    goto exit_error;
                }

                kv_item_header_t header = UNWRAP(maybe_header);
                // i += ITEM_HEADER_SIZE;
                size_t item_start = i;
                size_t item_key_start = item_start + ITEM_HEADER_SIZE;
                size_t item_value_start = item_key_start + header.key_length;
                size_t next_item_start = item_value_start + header.value_length;
                size_t remaining_after_item = sizeof (buffer) - item_start;
                size_t item_size = ITEM_HEADER_SIZE + header.key_length + header.value_length;

                if (header.key_length == item.key.length && sodium_memcmp(&buffer[item_key_start], item.key.data, header.key_length) == 0) {
                    // This is the matching key it needs to be removed from the buffer
                    // Current item start
                    memmove(&buffer[item_start], &buffer[next_item_start], remaining_after_item);
                    sodium_memzero(&buffer[sizeof (buffer) - item_size], item_size);

                    i = item_start;
                } else {
                    i = next_item_start;
                }
            }

            if (!item_written && remaining >= total_entry_length) {
                // Append the new item to this chunk
                // The start location to place the item at in the buffer should be i
                kv_empty_result_t maybe_result = __kv_write_item(&buffer, i, &item);
                if (IS_ERR(maybe_result)) {
                    SET_ERR(result, UNWRAP_ERR(maybe_result));
                    goto exit_error;
                }
                if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, buffer, sizeof (buffer), NULL, 0, read_tag) != 0) {
                    SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
                    goto exit_error;
                }
                bytes_written = fwrite(ct_buffer, sizeof (byte_t), sizeof (ct_buffer), write_file);
                if (bytes_written != sizeof (ct_buffer)) {
                    SET_ERR(result, FILE_WRITE_FAILED);
                    goto exit_error;
                }
            } else if (!item_written && read_tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
                // The item needs to be added as a new chunk
                if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, buffer, sizeof (buffer), NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_PUSH) != 0) {
                    SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
                    goto exit_error;
                }
                bytes_written = fwrite(ct_buffer, sizeof (byte_t), sizeof (ct_buffer), write_file);
                if (bytes_written != sizeof (ct_buffer)) {
                    SET_ERR(result, FILE_WRITE_FAILED);
                    goto exit_error;
                }
                sodium_memzero(buffer, sizeof (buffer));
                sodium_memzero(ct_buffer, sizeof (ct_buffer));
                kv_empty_result_t maybe_result = __kv_write_item(&buffer, 0, &item);
                if (IS_ERR(maybe_result)) {
                    SET_ERR(result, UNWRAP_ERR(maybe_result));
                    goto exit_error;
                }
                if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, buffer, sizeof (buffer), NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL) != 0) {
                    SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
                    goto exit_error;
                }
                bytes_written = fwrite(ct_buffer, sizeof (byte_t), sizeof (ct_buffer), write_file);
                if (bytes_written != sizeof (ct_buffer)) {
                    SET_ERR(result, FILE_WRITE_FAILED);
                    goto exit_error;
                }
            } else {
                // Write out the read buffer as is
                if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, buffer, sizeof (buffer), NULL, 0, read_tag) != 0) {
                    SET_ERR(result, FATAL_ENCRYPTION_FAILURE);
                    goto exit_error;
                }
                bytes_written = fwrite(ct_buffer, sizeof (byte_t), sizeof (ct_buffer), write_file);
                if (bytes_written != sizeof (ct_buffer)) {
                    SET_ERR(result, FILE_WRITE_FAILED);
                    goto exit_error;
                }
            }

            sodium_memzero(buffer, sizeof (buffer));
            sodium_memzero(ct_buffer, sizeof (ct_buffer));

        } while (!feof(kv->file) && read_tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL);
    } else {
        SET_ERR(result, FILE_READ_FAILED);
        goto exit_error;
    }
    
    // The write file has been successfully created.
    fclose(write_file);
    fclose(kv->file);

    if (remove(kv->read_filename) != 0) {
        SET_ERR(result, FILE_REMOVE_FAILED);
        goto exit_error;
    }
    if (rename(kv->write_filename, kv->read_filename) != 0) {
        SET_ERR(result, FILE_RENAME_FAILED);
        goto exit_error;
    }

    kv->file = fopen(kv->read_filename, "rb");
    if (kv->file == NULL) {
        SET_ERR(result, FILE_OPEN_FAILED);
        goto exit_error;
    }

    // seek to the start of the kv entries
    if (fseek(kv->file, kv->kv_start, SEEK_SET) != 0) {
        SET_ERR(result, FILE_SEEK_FAILED);
        goto exit_error;
    }

    kv->unlock_start_time_s = kv_util_get_monotonic_time();
    kv->unlocked = true;

    SET_OK_EMPTY(result);
    goto exit_success;

exit_success:
    goto cleanup;
exit_error:
    // We may potentially need to delete a partial 'write' file here
    goto cleanup;
cleanup:
    sodium_memzero(ct_header, sizeof (ct_header));
    sodium_memzero(ct_buffer, sizeof (ct_buffer));
    sodium_memzero(buffer, sizeof (buffer));
    sodium_memzero(&st_in, sizeof (st_in));
    sodium_memzero(&st_out, sizeof (st_out));

    return result;
}

// kv_empty_result_t kv_delete (kv_t* kv, kv_value_t key);
kv_empty_result_t kv_close  (kv_t* kv)
{
    kv_empty_result_t result = {0};

    sodium_memzero(kv->master_key, sizeof (kv->master_key));
    sodium_munlock(kv->master_key, sizeof (kv->master_key));

    if (!kv->unlocked) return OK_EMPTY(result);

    fclose(kv->file);

    return OK_EMPTY(result);
}