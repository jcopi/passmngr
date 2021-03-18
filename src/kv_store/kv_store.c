#include <kv_store.h>

#include <unistd.h>
#include <string.h>

#if defined(_POSIX_MONOTONIC_CLOCK)

    #define __USE_POSIX199309
    #include <time.h>

    double kv_util_get_monotonic_time ()
    {
        #if defined(CLOCK_MONOTONIC_RAW)
        #define CLOCK_ID CLOCK_MONOTONIC_RAW
        #elif defined(CLOCK_MONOTONIC)
        #define CLOCK_ID CLOCK_MONOTONIC
        #elif defined(CLOCK_BOOTTIME)
        #define CLOCK_ID CLOCK_BOOTTIME
        #endif

        struct timespec ts;
        clock_gettime(CLOCK_ID, &ts);
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
bool kv_util_is_past_expiration(double start_time, double max_time)
{
    double current_time = kv_util_get_monotomic_time();
    // attempt to check if clock has overflowed or if the elapsed time exceeds max
    return (current_time > start_time) && ((current_time - start_time) < max_time);
}

kv_empty_result_t kv_set_default_settings (kv_t* kv)
{

}

kv_empty_result_t kv_init(kv_t* kv, const char* const read_filename, const char* const write_filename)
{
    kv_empty_result_t result = {0};

    if (sodium_init() < 0) {
        return ERR(result, FATAL_OUT_OF_MEMORY);
    }

    if (sodium_mlock(kv->master_key, sizeof (kv->master_key)) != 0 || sodium_mlock(kv->value_buffer, sizeof (kv->value_buffer)) != 0) {
        return ERR(result, FATAL_OUT_OF_MEMORY);
    }

    sodium_memzero(kv->master_key, sizeof (kv->master_key));
    sodium_memzero(kv->value_buffer, sizeof (kv->value_buffer));

    kv->read_filename = read_filename;
    kv->write_filename = write_filename;

    kv->initialized = true;
}

kv_empty_result_t kv_open (kv_t* kv, const char * const password, uint64_t password_length)
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
        OK_EMPTY(result);
        goto exit_success;
    }

    // Sensitive buffers should be locked to prevent swapping to disk and being printed in kernel logs
    // ciphertexts and salts aren't secret so the don't strictly need to be locked
    if (sodium_mlock(password_key, sizeof (password_key)) != 0 || sodium_mlock(&st, sizeof (st)) != 0) {
        ERR(result, FATAL_OUT_OF_MEMORY);
        goto exit_error;
    }

    kv->max_unlock_time_s = DEFAULT_MAX_UNLOCK_TIME;

    if (access(kv->read_filename, F_OK) == 0) {
        // File exists, open it and begin reading the header
        kv->file = fopen(kv->read_filename, "rb");
        if (kv->file == NULL) {
            ERR(result, FILE_OPEN_FAILED);
            goto exit_error;
        }

        // read the master salt
        size_t nread = fread(master_salt, sizeof (byte_t), sizeof (master_salt), kv->file);
        if (nread != sizeof (master_salt)) {
            ERR(result, FILE_READ_FAILED);
            goto exit_error;
        }
        // Generate the password key
        if (crypto_pwhash(password_key, sizeof (password_key), password, password_length, master_salt, 
        crypto_pwhash_argon2id_OPSLIMIT_SENSITIVE, crypto_pwhash_argon2id_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
            ERR(result, FATAL_OUT_OF_MEMORY);
            goto exit_error;
        }

        // decrypt the master key and read it
        unsigned char tag;
        nread = fread(encrypted_key_header, sizeof (byte_t), sizeof (encrypted_key_header), kv->file);
        if (nread != sizeof (encrypted_key_header)) {
            ERR(result, FILE_READ_FAILED);
            goto exit_error;
        }
        if (crypto_secretstream_xchacha20poly1305_init_pull(&st, encrypted_key_header, password_key) != 0) {
            ERR(result, FATAL_ENCRYPTION_FAILURE);
            goto exit_error;
        }

        nread = fread(encrypted_key_ciphertext, sizeof (byte_t), sizeof (encrypted_key_ciphertext), kv->file);
        if (nread != sizeof (encrypted_key_ciphertext)) {
            ERR(result, FILE_READ_FAILED);
            goto exit_error;
        }
        if (crypto_secretstream_xchacha20poly1305_pull(&st, kv->master_key, NULL, &tag,
        encrypted_key_ciphertext, sizeof (encrypted_key_ciphertext), NULL, 0) != 0 ||
        tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
            ERR(result, FATAL_ENCRYPTION_FAILURE);
            goto exit_error;
        }

        kv->unlock_start_time_s = kv_util_get_monotomic_time();
        kv->unlocked = true;
        kv->kv_start = ftell(kv->file);

        goto exit_success;
    } else {
        // open the file for writing, normally writing would only happen on the `write_filename`
        // in this case since the read file name doesn't exist theres no need to do the write, delete, rename sequence
        kv->file = fopen(kv->read_filename, "wb");
        if (kv->file == NULL) {
            ERR(result, FILE_OPEN_FAILED);
            goto exit_error;
        }

        // generate and write a master salt
        randombytes_buf(master_salt, sizeof (master_salt));
        size_t nwritten = fwrite(master_salt, sizeof (byte_t), sizeof (master_salt), kv->file);
        if (nwritten != sizeof (master_salt)) {
            ERR(result, FILE_WRITE_FAILED);
            goto exit_error;
        }

        // generate the password key
        if (crypto_pwhash(password_key, sizeof (password_key), password, password_length, master_salt, 
        crypto_pwhash_argon2id_OPSLIMIT_SENSITIVE, crypto_pwhash_argon2id_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
            ERR(result, FATAL_OUT_OF_MEMORY);
            goto exit_error;
        }

        // Generate a master key and write it to a file
        crypto_secretstream_xchacha20poly1305_keygen(kv->master_key);
        
        unsigned char tag;
        if (crypto_secretstream_xchacha20poly1305_init_push(&st, encrypted_key_header, kv->master_key) != 0) {
            ERR(result, FATAL_ENCRYPTION_FAILURE);
            goto exit_error;
        }

        nwritten = fwrite(encrypted_key_header, sizeof (byte_t), sizeof (encrypted_key_header), kv->file);
        if (nwritten != sizeof (encrypted_key_header)) {
            ERR(result, FILE_WRITE_FAILED);
            goto exit_error;
        }
        
        tag = crypto_secretstream_xchacha20poly1305_TAG_FINAL;
        if (crypto_secretstream_xchacha20poly1305_push(&st, encrypted_key_ciphertext, NULL, kv->master_key, sizeof (kv->master_key), NULL, 0, tag) != 0) {
            ERR(result, FATAL_ENCRYPTION_FAILURE);
            goto exit_error;
        }

        nwritten = fwrite(encrypted_key_ciphertext, sizeof (byte_t), sizeof (encrypted_key_ciphertext), kv->file);
        if (nwritten != sizeof (encrypted_key_ciphertext)) {
            ERR(result, FILE_WRITE_FAILED);
            goto exit_error;
        }

        kv->kv_start = ftell(kv->file);
        kv->kv_empty = true;

        // close the file and reopen it for reading
        fclose(kv->file);
        kv->file = fopen(kv->read_filename, "rb");
        if (kv->file == NULL) {
            ERR(result, FILE_OPEN_FAILED);
            goto exit_error;
        }

        // seek to the start of the kv entries
        if (fseek(kv->file, kv->kv_start, SEEK_SET) != 0) {
            ERR(result, FILE_SEEK_FAILED);
            goto exit_error;
        }

        kv->unlock_start_time_s = kv_util_get_monotomic_time();
        kv->unlocked = true;

        goto exit_success;
    }

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

kv_value_result_t kv_get (kv_t* kv, kv_value_t key)
{
    assert(kv->initialized);

    kv_value_t value = {.data = NULL, .length = 0};
    kv_value_result_t result = {0};

    byte_t ct_read_buffer[CT_READ_BUFFER_LENGTH];
    byte_t ct_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    byte_t read_buffer[READ_BUFFER_LENGTH];
    crypto_secretstream_xchacha20poly1305_state st;

    size_t bytes_read;

    if (!kv->unlocked) {
        ERR(result, RUNTIME_NOT_OPEN);
        goto exit_error;
    }

    // cipher text buffer don't strictly need to be locked
    if (sodium_mlock(read_buffer, sizeof (read_buffer)) != 0 || sodium_mlock(&st, sizeof (st)) != 0) {
        ERR(result, FATAL_OUT_OF_MEMORY);
        goto exit_error;
    }

    // Attempt to seek to the start of the kv
    if (fseek(kv->file, kv->kv_start, SEEK_SET) != 0) {
        ERR(result, FILE_SEEK_FAILED);
        goto exit_error;
    }

    // If at the end of the file then no entries exist, exit
    if (feof(kv->file)) {
        ERR(result, RUNTIME_ITEM_NOT_FOUND);
        goto exit_error;
    }

    sodium_memzero(kv->value_buffer, sizeof (kv->value_buffer));

    // If there are any entries in the kv store it should have at least HEADERBYTES + ABYTES to read
    // Read the secretstream header
    bytes_read = fread(ct_header, sizeof (byte_t), sizeof (ct_header), kv->file);
    if (bytes_read != sizeof (ct_header)) {
        ERR(result, FILE_READ_FAILED);
        goto exit_error;
    }

    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, ct_header, kv->master_key) != 0) {
        ERR(result, FATAL_ENCRYPTION_FAILURE);
        goto exit_error;
    }
    
    // If there is not data after the header this will be treated as an error.
    // If the kv is empty the header should not be populated
    if (feof(kv->file)) {
        ERR(result, FATAL_MALFORMED_FILE);
        goto exit_error;
    }

    unsigned char tag;
    do {
        bytes_read = fread(ct_read_buffer, sizeof (byte_t), sizeof (ct_read_buffer), kv->file);
        if (bytes_read != sizeof (ct_read_buffer)) {
            // Data will always be padded (with 0s) to a multiple of the default chunk size
            // That makes this case an error and eliminates the need to handle this particular edge
            ERR(result, FATAL_MALFORMED_FILE);
            goto exit_error;
        }
        // In addition to padding the end of the kv to a multiple of chunk size, the kv should be padded such that no entry overlaps a chunk.
        // In practice this means that the total length of the key and value cannot exceed CHUNK SIZE - 24.
        // This is a substantial limitation, but in the typical usage it should not be particularly detrimental. 
        if (crypto_secretstream_xchacha20poly1305_pull(&st, read_buffer, NULL, &tag, ct_read_buffer, sizeof (ct_read_buffer), NULL, 0) != 0) {
            ERR(result, FATAL_ENCRYPTION_FAILURE);
            goto exit_error;
        }

        size_t i = 0;
        uint64_t key_length;
        uint64_t value_length;
        uint64_t timestamp;

        while (i < sizeof (read_buffer)) {
            size_t remaining = sizeof (read_buffer) - i;
            if (remaining <= (sizeof (key_length) + sizeof (value_length) + sizeof (timestamp)) || sodium_is_zero(&read_buffer[i], remaining)) {
                // An entry could not fit here or all remaining bytes are zero, so the rest must be padded.
                break;
            }

            memcpy(&key_length, &read_buffer[i], sizeof (key_length));
            i += sizeof (key_length);
            memcpy(&value_length, &read_buffer[i], sizeof (value_length));
            i += sizeof (value_length);
            memcpy(&timestamp, &read_buffer[i], sizeof (timestamp));
            i += sizeof (timestamp);

            remaining = sizeof (read_buffer) - i;
            if (key_length == 0 || value_length == 0 || key_length >= sizeof (read_buffer) || value_length >= sizeof (kv->value_buffer) || (key_length + value_length) > remaining) {
                // this is an invalid key or value length, the file is malformed and an error should be raised
                ERR(result, FATAL_MALFORMED_FILE);
                goto exit_error;
            } else if (key_length == key.length && sodium_memcmp(&read_buffer[i], key.data, key_length) == 0) {
                // This is the matching key, the value needs to be returned.
                i += key_length;
                memcpy(kv->value_buffer, &read_buffer[i], value_length);
                value.data = kv->value_buffer;
                value.length = value_length;
                OK(result, value);
                goto exit_success;
            } else {
                i += key_length + value_length;
            }
        }
    } while (!feof(kv->file) && tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL);

    ERR(result, RUNTIME_ITEM_NOT_FOUND);
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

kv_empty_result_t kv_set    (kv_t* kv, kv_value_t key, kv_value_t value)
{
    // set works similarly to get, but values can't be inserted into the middle of an encrypted kv
    // the initial header (salt & encrypted master) need to be copied over as is to the `write_filename`
    // the data in the kv needs to be read, decrypted, modified (if applicable), re-encrypted, then written to the new file

    assert(kv->initialized);

    kv_empty_result_t result = {0};

    byte_t ct_read_buffer[CT_READ_BUFFER_LENGTH];
    byte_t ct_write_buffer[CT_READ_BUFFER_LENGTH];
    byte_t ct_header_in[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    byte_t ct_header_out[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    byte_t read_buffer[READ_BUFFER_LENGTH];
    byte_t write_buffer[READ_BUFFER_LENGTH];
    crypto_secretstream_xchacha20poly1305_state st_in;
    crypto_secretstream_xchacha20poly1305_state st_out;

    size_t bytes_read;
    size_t bytes_written;

    if (!kv->unlocked) {
        ERR(result, RUNTIME_NOT_OPEN);
        goto exit_error;
    }

    // cipher text buffer don't strictly need to be locked
    if (sodium_mlock(read_buffer, sizeof (read_buffer)) != 0 || sodium_mlock(&st_in, sizeof (st_in)) != 0 ||
        sodium_mlock(read_buffer, sizeof (write_buffer)) != 0 || sodium_mlock(&st_out, sizeof (st_out)) != 0) {
        ERR(result, FATAL_OUT_OF_MEMORY);
        goto exit_error;
    }

    // Attempt to seek to the start of the read file
    if (fseek(kv->file, 0, SEEK_SET) != 0) {
        ERR(result, FILE_SEEK_FAILED);
        goto exit_error;
    }

    // The first step is to create a new file and copy the master salt and encrypted password.
    FILE* write_file = fopen(kv->write_filename, "wb");
    if (write_file == NULL) {
        ERR(result, FILE_OPEN_FAILED);
        goto exit_error;
    }

    // Read the entire starting header from the read file and write it to the write file
    byte_t header_buffer[crypto_pwhash_argon2id_SALTBYTES + crypto_secretstream_xchacha20poly1305_HEADERBYTES + crypto_secretstream_xchacha20poly1305_KEYBYTES + crypto_secretstream_xchacha20poly1305_ABYTES];
    bytes_read = fread(header_buffer, sizeof (byte_t), sizeof (header_buffer), kv->file);
    if (bytes_read != sizeof (header_buffer)) {
        ERR(result, FILE_READ_FAILED);
        goto exit_error;
    }
    bytes_written = fwrite(header_buffer, sizeof (byte_t), sizeof (header_buffer), write_file);
    if (bytes_written != sizeof (header_buffer)) {
        ERR(result, FILE_WRITE_FAILED);
        goto exit_error;
    }

    // Read and write the secretstream headers
    bytes_read = fread(ct_header_in, sizeof (byte_t), sizeof (ct_header_in), kv->file);
    if (bytes_read != sizeof (ct_header_in)) {
        ERR(result, FILE_READ_FAILED);
        goto exit_error;
    }
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st_in, ct_header_in, kv->master_key) != 0) {
        ERR(result, FATAL_ENCRYPTION_FAILURE);
        goto exit_error;
    }
    
    if (crypto_secretstream_xchacha20poly1305_init_push(&st_out, ct_header_out, kv->master_key) != 0) {
        ERR(result, FATAL_ENCRYPTION_FAILURE);
        goto exit_error;
    }
    bytes_written = fwrite(ct_header_out, sizeof (byte_t), sizeof (ct_header_out), write_file);
    if (bytes_written != sizeof (ct_header_out)) {
        ERR(result, FILE_WRITE_FAILED);
        goto exit_error;
    }

    unsigned char read_tag;
    unsigned char write_tag;

    size_t write_buffer_i = 0;
    do {
        bytes_read = fread(ct_read_buffer, sizeof (byte_t), sizeof (ct_read_buffer), kv->file);
        if (bytes_read != sizeof (ct_read_buffer)) {
            // Data will always be padded (with 0s) to a multiple of the default chunk size
            // That makes this case an error and eliminates the need to handle this particular edge
            ERR(result, FATAL_MALFORMED_FILE);
            goto exit_error;
        }
        // In addition to padding the end of the kv to a multiple of chunk size, the kv should be padded such that no entry overlaps a chunk.
        // In practice this means that the total length of the key and value cannot exceed CHUNK SIZE - 24.
        // This is a substantial limitation, but in the typical usage it should not be particularly detrimental. 
        if (crypto_secretstream_xchacha20poly1305_pull(&st, read_buffer, NULL, &tag, ct_read_buffer, sizeof (ct_read_buffer), NULL, 0) != 0) {
            ERR(result, FATAL_ENCRYPTION_FAILURE);
            goto exit_error;
        }

        size_t i = 0;
        uint64_t key_length;
        uint64_t value_length;
        uint64_t timestamp;

        while (i < sizeof (read_buffer)) {
            size_t remaining = sizeof (read_buffer) - i;
            if (remaining <= (sizeof (key_length) + sizeof (value_length) + sizeof (timestamp)) || sodium_is_zero(&read_buffer[i], remaining)) {
                // An entry could not fit here or all remaining bytes are zero, so the rest must be padded.
                break;
            }

            memcpy(&key_length, &read_buffer[i], sizeof (key_length));
            i += sizeof (key_length);
            memcpy(&value_length, &read_buffer[i], sizeof (value_length));
            i += sizeof (value_length);

            // The timestamp is currently not in use
            memcpy(&timestamp, &read_buffer[i], sizeof (timestamp));
            i += sizeof (timestamp);

            remaining = sizeof (read_buffer) - i;
            if (key_length == 0 || value_length == 0 || key_length >= sizeof (read_buffer) || value_length >= sizeof (kv->value_buffer) || (key_length + value_length) > remaining) {
                // this is an invalid key or value length, the file is malformed and an error should be raised
                ERR(result, FATAL_MALFORMED_FILE);
                goto exit_error;
            } else if (key_length == key.length && sodium_memcmp(&read_buffer[i], key.data, key_length) == 0) {
                // This is the matching key, the value needs to be updated.
                i += key_length;
                memcpy(kv->value_buffer, &read_buffer[i], value_length);
                value.data = kv->value_buffer;
                value.length = value_length;
                
                
            } else {
                i += key_length + value_length;
            }
        }
    } while (!feof(kv->file) && tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL);

exit_success:
    goto cleanup;
exit_error:
    goto cleanup;
cleanup:
    sodium_memzero(ct_header_in, sizeof (ct_header_in));
    sodium_memzero(ct_header_out, sizeof (ct_header_out));
    sodium_memzero(ct_read_buffer, sizeof (ct_read_buffer));
    sodium_memzero(ct_write_buffer, sizeof (ct_write_buffer));
    sodium_memzero(read_buffer, sizeof (read_buffer));
    sodium_memzero(write_buffer, sizeof (write_buffer));
    sodium_memzero(&st_in, sizeof (st_in));
    sodium_memzero(&st_out, sizeof (st_out));

    return result;
}

kv_empty_result_t kv_delete (kv_t* kv, kv_value_t key);
kv_empty_result_t kv_close  (kv_t* kv);