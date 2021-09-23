#include <kv.h>

#include <string.h>
#include <errno.h>
#include <time.h>

kv_error_t __kv_gen_error (int32_t code);
kv_error_t __kv_gen_success ();

inline kv_error_t __kv_gen_error (int32_t code) {
    return (kv_error_t){.is_error=true, .code=code};
}
inline kv_error_t __kv_gen_success () {
    return (kv_error_t){.is_error=false};
}

result_uint64_t __kv_get_time_ms () {
    struct timespec ts;
    int result = timespec_get(&ts, TIME_UTC);
    if (result != TIME_UTC) {
        return result_uint64_error(__kv_gen_error(FAILED_TO_GET_TIME));
    }
    return result_uint64_ok((uint64_t)(1000 * ts.tv_sec) + (uint64_t)(ts.tv_nsec / 1000000));
}

kv_error_t __kv_make_readonly (const char* const filename) {
    #if defined(__linux__)
        #include <sys/stat.h>
        chmod(filename, S_IRUSR|S_IRGRP|S_IROTH);
    #elif defined(_WIN32)

    #elif defined(__APPLE__)
    
    #else

    #endif

    return __kv_gen_success();
}

uint64_t __kv_gen_reservation () {
    uint64_t result;
    randombytes_buf(&result, sizeof (result));

    return result;
}

size_t __kv_item_total_size (kv_item_t item) {
    return SIZEOF_ITEM_HEADER + item.key.len + item.value.len;
}

size_t __kv_blk_consumer_bytes_remaining (kv_block_t* blk) {
    return blk->bytes_produced - blk->bytes_consumed;
}

size_t __kv_blk_producer_space_remaining (kv_block_t* blk) {
    return sizeof (blk->buffer) - blk->bytes_produced;
}

result_opt_item_t __kv_blk_consume_next_item (kv_block_t* blk) {
    kv_item_header_t item_header = {0};
    size_t bytes_remaining = __kv_blk_consumer_bytes_remaining(blk);
    if (sodium_is_zero(blk->buffer + blk->bytes_consumed, bytes_remaining)) {
        return result_opt_item_ok(optional_item_empty());
    } else if (bytes_remaining < MIN_ITEM_SIZE) {
        // It is impossible for another item to fit, but the remaining bytes are not zero
        // This could possible be treated as an error
        return result_opt_item_ok(optional_item_empty());
    } else {
        memcpy(&item_header.key_length, blk->buffer + blk->bytes_consumed, sizeof (item_header.key_length));
        blk->bytes_consumed += sizeof (item_header.key_length);
        bytes_remaining = __kv_blk_consumer_bytes_remaining(blk);
        memcpy(&item_header.value_length, blk->buffer + blk->bytes_consumed, sizeof (item_header.value_length));
        blk->bytes_consumed += sizeof (item_header.value_length);
        bytes_remaining = __kv_blk_consumer_bytes_remaining(blk);
        memcpy(&item_header.timestamp, blk->buffer + blk->bytes_consumed, sizeof (item_header.timestamp));
        blk->bytes_consumed += sizeof (item_header.timestamp);
        bytes_remaining = __kv_blk_consumer_bytes_remaining(blk);

        if (bytes_remaining < item_header.key_length + item_header.value_length) {
            return result_opt_item_error(__kv_gen_error(MALFORMED_FILE));
        }

        slice_t key_slice = s_create(blk->buffer + blk->bytes_consumed, item_header.key_length, item_header.key_length);
        blk->bytes_consumed += item_header.key_length;
        slice_t value_slice = s_create(blk->buffer + blk->bytes_consumed, item_header.value_length, item_header.value_length);
        blk->bytes_consumed += item_header.value_length;

        return result_opt_item_ok(optional_item_ok((kv_item_t){.key=key_slice, .value=value_slice, .timestamp=item_header.timestamp}));
    }
}

kv_error_t __kv_blk_produce_next_item (kv_block_t* blk, kv_item_t item) {
    size_t bytes_remaining = __kv_blk_producer_space_remaining(blk);
    size_t item_size = __kv_item_total_size(item);

    if (item_size > bytes_remaining || item.key.len > MAX_ITEM_KEY_SIZE || item.value.len > MAX_ITEM_VALUE_SIZE) {
        return __kv_gen_error(ITEM_TOO_LARGE);
    }

    kv_item_header_t header = (kv_item_header_t){.key_length=item.key.len, .value_length=item.value.len, .timestamp=item.timestamp};

    memmove(blk->buffer + blk->bytes_produced, &header.key_length, sizeof (header.key_length));
    blk->bytes_produced += sizeof (header.key_length);
    memmove(blk->buffer + blk->bytes_produced, &header.value_length, sizeof (header.value_length));
    blk->bytes_produced += sizeof (header.value_length);
    memmove(blk->buffer + blk->bytes_produced, &header.timestamp, sizeof (header.timestamp));
    blk->bytes_produced += sizeof (header.timestamp);
    memmove(blk->buffer + blk->bytes_produced, item.key.ptr, header.key_length);
    blk->bytes_produced += header.key_length;
    memmove(blk->buffer + blk->bytes_produced, item.value.ptr, header.value_length);
    blk->bytes_produced += header.value_length;

    return __kv_gen_success();
}

void __kv_blk_producer_pad_remaining (kv_block_t* blk) {
    size_t bytes_remaining = __kv_blk_producer_space_remaining(blk);
    sodium_memzero(blk->buffer + blk->bytes_produced, bytes_remaining);
    blk->bytes_produced += bytes_remaining;
}

void __kv_blk_set_filled (kv_block_t* blk) {
    blk->bytes_produced = sizeof (blk->buffer);
    blk->bytes_consumed = 0;
}

void __kv_blk_clear (kv_block_t* blk) {
    sodium_memzero(blk->buffer, sizeof (blk->buffer));
    blk->bytes_produced = 0;
    blk->bytes_consumed = 0;
}

kv_error_t __kv_blk_read_full_block (kv_block_t* blk, FILE* file) {
    __kv_blk_clear(blk);
    size_t bytes_read = fread(blk->buffer, 1, sizeof (blk->buffer), file);
    if (bytes_read != sizeof (blk->buffer)) {
        return __kv_gen_error(errno);
    }

    blk->bytes_produced = sizeof (blk->buffer);
    blk->bytes_consumed = 0;
    return __kv_gen_success();
}

kv_error_t __kv_blk_write_full_block (kv_block_t* blk, FILE* file) {
    size_t bytes_written = fwrite(blk->buffer, 1, blk->bytes_produced, file);
    if (bytes_written != blk->bytes_produced) {
        return __kv_gen_error(errno);
    }

    blk->bytes_consumed = blk->bytes_produced;
    return __kv_gen_success();
}

bool __kv_key_prefix_matches (slice_t prefix, slice_t key) {
    // The prefix can't be larger than the key
    if (prefix.len > key.len) {
        return false;
    }
    int result = strncmp((char*)prefix.ptr, (char*)key.ptr, prefix.len);
    return result == 0;
}
bool __kv_key_matches (slice_t prefix, slice_t key) {
    // The prefix can't be larger than the key
    if (prefix.len != key.len) {
        return false;
    }
    int result = strncmp((char*)prefix.ptr, (char*)key.ptr, prefix.len);
    return result == 0;
}

kv_error_t kv_init (kv_t* kv, slice_t read_filename, slice_t write_filename) {
    assert(!kv->initialized);

    *kv = (kv_t){0};

    if (read_filename.len > MAX_PATH_LENGTH) {
        return __kv_gen_error(FILE_PATH_TOO_LONG);
    }
    if (write_filename.len > MAX_PATH_LENGTH) {
        return __kv_gen_error(FILE_PATH_TOO_LONG);
    }

    sodium_memzero(kv->key, sizeof (kv->key));
    sodium_memzero(kv->block.buffer, sizeof (kv->block.buffer));

    // copy the contents from the current slices to the kv buffers
    slice_t main_fname_s = s_create(kv->main_filename, sizeof (kv->main_filename), 0);
    slice_t tmp_fname_s = s_create(kv->tmp_filename, sizeof (kv->tmp_filename), 0);

    s_append(main_fname_s, read_filename, read_filename.len);
    s_append(tmp_fname_s, write_filename, write_filename.len);

    if (sodium_mlock(kv->key, sizeof (kv->key)) != 0) {
        return __kv_gen_error(OUT_OF_MEMORY);
    }

    kv->block = (kv_block_t){0};
    if (sodium_mlock(kv->block.buffer, sizeof (kv->block.buffer)) != 0) {
        return __kv_gen_error(OUT_OF_MEMORY);
    }

    kv->initialized = true;
    kv->unlocked = false;
    kv->reserved = false;

    kv->max_unlock_time = DEFAULT_UNLOCK_DURATION;

    return __kv_gen_success();
}

kv_error_t kv_create (kv_t* kv) {
    assert(kv->initialized);
    assert(!kv->reserved);

    if (!kv->unlocked) {
        return __kv_gen_error(KV_IS_LOCKED);
    }
    result_uint64_t maybe_time = __kv_get_time_ms();
    if (maybe_time.is_error) {
        return result_uint64_unwrap_err(maybe_time);
    }

    uint64_t current_time = result_uint64_unwrap(maybe_time);

    uint64_t duration = current_time - kv->unlock_time;
    if (duration >= kv->max_unlock_time) {
        kv_lock(kv);
        return __kv_gen_error(KV_IS_LOCKED);
    }

    crypto_secretstream_xchacha20poly1305_state st_out = {0};
    uint8_t header_buffer[crypto_secretstream_xchacha20poly1305_HEADERBYTES] = {0};
    if (sodium_mlock(&st_out, sizeof (st_out)) != 0) {
        return __kv_gen_error(OUT_OF_MEMORY);
    }

    FILE* file_out = fopen(kv->tmp_filename, "wb");
    if (file_out == NULL) {
        return __kv_gen_error(errno);
    }

    __kv_blk_clear(&kv->block);
    __kv_blk_producer_pad_remaining(&kv->block);

    #define ERROR_EXIT() do { \
        fclose(file_out); \
        remove(kv->tmp_filename); \
        __kv_blk_clear(&kv->block); \
        sodium_memzero(&st_out, sizeof (st_out)); \
        sodium_munlock(&st_out, sizeof (st_out)); \
    } while (false)

    kv_error_t err = __kv_blk_write_full_block(&kv->block, file_out);
    if (err.is_error) {
        ERROR_EXIT();
        return err;
    }

    sodium_memzero(header_buffer, sizeof (header_buffer));

    if (crypto_secretstream_xchacha20poly1305_init_push(&st_out, header_buffer, kv->key) != 0) {
        ERROR_EXIT();
        return __kv_gen_error(CRYPTO_ERROR);
    }
    size_t bytes_written = fwrite(header_buffer, 1, sizeof (header_buffer), file_out);
    if (bytes_written != sizeof (header_buffer)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }

    char* item_key = "/init";
    size_t item_key_len = strlen(item_key);
    uint64_t item_value = __kv_gen_reservation();
    uint8_t ct_buffer[KV_CIPHERTEXT_BLOCK_SIZE] = {0};

    kv_item_t item_to_write = (kv_item_t){.key=s_create(&item_key, item_key_len, item_key_len), .value=s_create(&item_value, sizeof (item_value), sizeof (item_value)), .timestamp=current_time};

    __kv_blk_clear(&kv->block);

    err = __kv_blk_produce_next_item(&kv->block, item_to_write);
    if (err.is_error) {
        ERROR_EXIT();
        return err;
    }

    sodium_memzero(ct_buffer, sizeof (ct_buffer));
    __kv_blk_producer_pad_remaining(&kv->block);
    if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, kv->block.buffer, sizeof (kv->block.buffer), NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL) != 0) {
        ERROR_EXIT();
        return __kv_gen_error(CRYPTO_ERROR);
    }
    bytes_written = fwrite(ct_buffer, 1, sizeof (ct_buffer), file_out);
    if (bytes_written != sizeof (ct_buffer)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }
    sodium_memzero(ct_buffer, sizeof (ct_buffer));

    fclose(file_out);

    __kv_blk_clear(&kv->block);

    sodium_memzero(&st_out, sizeof (st_out));
    sodium_munlock(&st_out, sizeof (st_out));

    if (rename(kv->tmp_filename, kv->main_filename) != 0) {
        // This case should be handled in a special manner
        // No data loss has occured, but the data is in a different file location than expected
        return __kv_gen_error(RENAME_TMP_TO_MAIN_FAILED);
    }

    return __kv_gen_success();

    #undef ERROR_EXIT
}

slice_t kv_key_buffer (kv_t* kv) {
    assert(kv->initialized);
    assert(!kv->unlocked);

    sodium_memzero(kv->key, sizeof (kv->key));

    return s_create(kv->key, sizeof (kv->key), 0);
}

kv_error_t kv_unlock (kv_t* kv) {
    assert(kv->initialized);

    if (kv->unlocked) {
        return __kv_gen_success();
    }

    result_uint64_t maybe_time = __kv_get_time_ms();
    if (maybe_time.is_error) {
        return result_uint64_unwrap_err(maybe_time);
    }

    kv->unlock_time = result_uint64_unwrap(maybe_time);
    kv->unlocked = true;

    return __kv_gen_success();
}
kv_error_t kv_lock (kv_t* kv) {
    assert(kv->initialized);

    sodium_memzero(kv->key, sizeof (kv->key));

    kv->unlocked = false;

    return __kv_gen_success();
}

result_uint64_t kv_reserve_get (kv_t* kv) {
    assert(kv->initialized);
    assert(!kv->reserved);

    uint64_t reservation = __kv_gen_reservation();

    kv->reserved = true;
    kv->reservation = reservation;

    return result_uint64_ok(reservation);
}
kv_error_t kv_release_get (kv_t* kv, uint64_t reservation) {
    assert(kv->initialized);
    assert(kv->reserved);
    assert(kv->reservation == reservation);

    kv->reserved = false;
    kv->reservation = 0;

    __kv_blk_clear(&kv->block);

    return __kv_gen_success();
}

result_opt_item_t kv_plain_get (kv_t* kv, uint64_t reservation, slice_t key, size_t n) {
    assert(kv->initialized);
    assert(kv->reserved);
    assert(kv->reservation == reservation);

    FILE* file = fopen(kv->main_filename, "rb");
    if (file == NULL) {
        // Maybe use errno here instead
        return result_opt_item_error(__kv_gen_error(errno));
    }

    __kv_blk_clear(&kv->block);
    kv_error_t err = __kv_blk_read_full_block(&kv->block, file);
    if (err.is_error) {
        fclose(file);
        __kv_blk_clear(&kv->block);
        return result_opt_item_error(__kv_gen_error(errno));
    }

    // In the plaintext case, the entire KV store must fit into a single block,
    // so the file can be closed as soon as the block is read
    fclose(file);

    size_t matches_found = 0;
    while (true) {
        result_opt_item_t maybe_opt_item = __kv_blk_consume_next_item(&kv->block);
        if (maybe_opt_item.is_error) {
            __kv_blk_clear(&kv->block);
            return result_opt_item_error(result_opt_item_unwrap_err(maybe_opt_item));
        }

        optional_item_t maybe_item = result_opt_item_unwrap(maybe_opt_item);
        if (maybe_item.is_available) {
            kv_item_t item = optional_item_unwrap(maybe_item);
            if (__kv_key_prefix_matches(key, item.key)) {
                if (matches_found == n) {
                    // Don't zero the block here, this memory is referenced by the item
                    //  __kv_blk_clear(&kv->block);
                    return result_opt_item_ok(optional_item_ok(item));
                }
            }
        } else {
            // This is the end of the block
            // since the item was not found we can simply return an empty option
            __kv_blk_clear(&kv->block);
            return result_opt_item_ok(optional_item_empty());
        }
    }
}
result_opt_item_t kv_get (kv_t* kv, uint64_t reservation, slice_t key, size_t n) {
    assert(kv->initialized);
    assert(kv->reserved);
    assert(kv->reservation == reservation);

    if (!kv->unlocked) {
        return result_opt_item_error(__kv_gen_error(KV_IS_LOCKED));
    }
    result_uint64_t maybe_time = __kv_get_time_ms();
    if (maybe_time.is_error) {
        return result_opt_item_error(result_uint64_unwrap_err(maybe_time));
    }

    uint64_t duration = result_uint64_unwrap(maybe_time) - kv->unlock_time;
    if (duration >= kv->max_unlock_time) {
        kv_lock(kv);
        return result_opt_item_error(__kv_gen_error(KV_IS_LOCKED));
    }

    crypto_secretstream_xchacha20poly1305_state st = {0};
    uint8_t header_buffer[crypto_secretstream_xchacha20poly1305_HEADERBYTES] = {0};
    if (sodium_mlock(&st, sizeof (st)) != 0) {
        return result_opt_item_error(__kv_gen_error(OUT_OF_MEMORY));
    }

    FILE* file = fopen(kv->main_filename, "rb");
    if (file == NULL) {
        // Maybe use errno here instead
        return result_opt_item_error(__kv_gen_error(errno));
    }
    if (fseek(file, KV_BLOCK_SIZE, SEEK_SET) != 0) {
        fclose(file);
        sodium_memzero(&st, sizeof (st));
        sodium_munlock(&st, sizeof (st));
        return result_opt_item_error(__kv_gen_error(errno));
    }

    size_t bytes_read = fread(header_buffer, 1, sizeof (header_buffer), file);
    if (bytes_read != sizeof (header_buffer)) {
        fclose(file);
        sodium_memzero(&st, sizeof (st));
        sodium_munlock(&st, sizeof (st));
        return result_opt_item_error(__kv_gen_error(errno));
    }
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header_buffer, kv->key) != 0) {
        fclose(file);
        sodium_memzero(&st, sizeof (st));
        sodium_munlock(&st, sizeof (st));
        return result_opt_item_error(__kv_gen_error(CRYPTO_ERROR));
    }

    uint8_t ct_buffer[KV_CIPHERTEXT_BLOCK_SIZE] = {0};

    while (true) {
        // read ciphertext block
        if (feof(file)) {
            // At the end of the file. The item was not found
            fclose(file);
            sodium_memzero(&st, sizeof (st));
            sodium_munlock(&st, sizeof (st));
            __kv_blk_clear(&kv->block);
            return result_opt_item_ok(optional_item_empty());
        }

        size_t bytes_read = fread(ct_buffer, 1, sizeof (ct_buffer), file);
        if (bytes_read != sizeof (ct_buffer)) {
            // This is a read failure or a malformed file
            fclose(file);
            sodium_memzero(&st, sizeof (st));
            sodium_munlock(&st, sizeof (st));
            __kv_blk_clear(&kv->block);
            return result_opt_item_error(__kv_gen_error(errno));
        }

        __kv_blk_clear(&kv->block);

        unsigned char tag;
        if (crypto_secretstream_xchacha20poly1305_pull(&st, kv->block.buffer, NULL, &tag, ct_buffer, sizeof (ct_buffer), NULL, 0) != 0) {
            fclose(file);
            __kv_blk_clear(&kv->block);
            sodium_memzero(&st, sizeof (st));
            sodium_munlock(&st, sizeof (st));
            return result_opt_item_error(__kv_gen_error(CRYPTO_ERROR));
        }
   
        __kv_blk_set_filled(&kv->block);

        size_t matches_found = 0;
        while (true) {
            result_opt_item_t maybe_opt_item = __kv_blk_consume_next_item(&kv->block);
            if (maybe_opt_item.is_error) {
                fclose(file);
                __kv_blk_clear(&kv->block);
                sodium_memzero(&st, sizeof (st));
                sodium_munlock(&st, sizeof (st));
                return result_opt_item_error(result_opt_item_unwrap_err(maybe_opt_item));
            }

            optional_item_t maybe_item = result_opt_item_unwrap(maybe_opt_item);
            if (maybe_item.is_available) {
                kv_item_t item = optional_item_unwrap(maybe_item);
                if (__kv_key_prefix_matches(key, item.key)) {
                    if (matches_found == n) {
                        // Don't zero the block here, this memory is referenced by the item
                        fclose(file);
                        //__kv_blk_clear(&kv->block);
                        sodium_memzero(&st, sizeof (st));
                        sodium_munlock(&st, sizeof (st));
                        return result_opt_item_ok(optional_item_ok(item));
                    }
                }
            } else if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
                // This is the end of the block
                // since the item was not found we can simply return an empty option
                fclose(file);
                __kv_blk_clear(&kv->block);
                sodium_memzero(&st, sizeof (st));
                sodium_munlock(&st, sizeof (st));
                return result_opt_item_ok(optional_item_empty());
            }
        }
    }
}
// result_opt_item_t kv_get_sorted (kv_t* kv, uint64_t reservation, slice_t key, size_t n, int cmp (slice_t a, slice_t b), slice_t after);

kv_error_t kv_plain_set (kv_t* kv, slice_t key, slice_t value) {
    assert(kv->initialized);
    assert(!kv->reserved);

    kv_block_t tmp_blk = {0};
    __kv_blk_clear(&tmp_blk);

    result_uint64_t maybe_time = __kv_get_time_ms();
    if (maybe_time.is_error) {
        return result_uint64_unwrap_err(maybe_time);
    }

    uint64_t current_time = result_uint64_unwrap(maybe_time);

    FILE* file = fopen(kv->main_filename, "rb");
    if (file == NULL) {
        // Maybe use errno here instead
        return __kv_gen_error(errno);
    }
    FILE* file_out = fopen(kv->tmp_filename, "wb");
    if (file_out == NULL) {
        fclose(file);
        return __kv_gen_error(errno);
    }

    #define ERROR_EXIT() do {\
        fclose(file); \
        fclose(file_out); \
        remove(kv->tmp_filename); \
        __kv_blk_clear(&kv->block); \
        __kv_blk_clear(&tmp_blk); \
    } while (false)

    __kv_blk_clear(&kv->block);
    kv_error_t err = __kv_blk_read_full_block(&kv->block, file);
    if (err.is_error) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }

    kv_item_t item_to_write = (kv_item_t){.key=key, .value=value, .timestamp=current_time};

    while (true) {
        result_opt_item_t maybe_opt_item = __kv_blk_consume_next_item(&kv->block);
        if (maybe_opt_item.is_error) {
            ERROR_EXIT();
            return result_opt_item_unwrap_err(maybe_opt_item);
        }

        optional_item_t maybe_item = result_opt_item_unwrap(maybe_opt_item);
        if (maybe_item.is_available) {
            kv_item_t item = optional_item_unwrap(maybe_item);
            if (!__kv_key_matches(key, item.key)) {
                kv_error_t err = __kv_blk_produce_next_item(&tmp_blk, item);
                if (err.is_error) {
                    ERROR_EXIT();
                    return err;
                }
            }
        } else {
            // This is the end of the block
             __kv_blk_clear(&kv->block);

            err = __kv_blk_produce_next_item(&tmp_blk, item_to_write);
            if (err.is_error) {
                ERROR_EXIT();
                return err;
            }
            __kv_blk_producer_pad_remaining(&tmp_blk);

            err = __kv_blk_write_full_block(&tmp_blk, file_out);
            if (err.is_error) {
                ERROR_EXIT();
                return err;
            }

            uint8_t buffer[8192] = {0};
            size_t bytes_read = 0;
            while (true) {
                bytes_read = fread(buffer, 1, sizeof (buffer), file);
                if (bytes_read > 0) {
                    size_t bytes_written = fwrite(buffer, 1, bytes_read, file_out);
                    if (bytes_written != bytes_read) {
                        ERROR_EXIT();
                        return __kv_gen_error(errno);
                    }
                }

                if (bytes_read < sizeof (buffer) || feof(file)) {
                    break;
                }
            }
            

           
            fclose(file);
            fclose(file_out);

            __kv_blk_clear(&kv->block);
            __kv_blk_clear(&tmp_blk);

            if (remove(kv->main_filename) != 0) {
                remove(kv->tmp_filename);
                return __kv_gen_error(errno);
            }

            if (rename(kv->tmp_filename, kv->main_filename) != 0) {
                // This case should be handled in a special manner
                // No data loss has occured, but the data is in a different file location than expected
                return __kv_gen_error(RENAME_TMP_TO_MAIN_FAILED);
            }

            return __kv_gen_success();
        }
    }
    #undef ERROR_EXIT
}
kv_error_t kv_set (kv_t* kv, slice_t key, slice_t value) {
    assert(kv->initialized);
    assert(!kv->reserved);

    if (!kv->unlocked) {
        return __kv_gen_error(KV_IS_LOCKED);
    }
    result_uint64_t maybe_time = __kv_get_time_ms();
    if (maybe_time.is_error) {
        return result_uint64_unwrap_err(maybe_time);
    }

    uint64_t current_time = result_uint64_unwrap(maybe_time);

    uint64_t duration = current_time - kv->unlock_time;
    if (duration >= kv->max_unlock_time) {
        kv_lock(kv);
        return __kv_gen_error(KV_IS_LOCKED);
    }

    crypto_secretstream_xchacha20poly1305_state st = {0};
    crypto_secretstream_xchacha20poly1305_state st_out = {0};
    uint8_t header_buffer[crypto_secretstream_xchacha20poly1305_HEADERBYTES] = {0};
    if (sodium_mlock(&st, sizeof (st)) != 0 || sodium_mlock(&st_out, sizeof (st_out)) != 0) {
        return __kv_gen_error(OUT_OF_MEMORY);
    }
    kv_block_t tmp_blk = {0};
    __kv_blk_clear(&tmp_blk);
    if (sodium_mlock(tmp_blk.buffer, sizeof (tmp_blk.buffer)) != 0) {
        return __kv_gen_error(OUT_OF_MEMORY);
    }

    FILE* file = fopen(kv->main_filename, "rb");
    if (file == NULL) {
        // Maybe use errno here instead
        return __kv_gen_error(errno);
    }
    FILE* file_out = fopen(kv->tmp_filename, "wb");
    if (file_out == NULL) {
        fclose(file);
        return __kv_gen_error(errno);
    }

    #define ERROR_EXIT() do {\
        fclose(file); \
        fclose(file_out); \
        remove(kv->tmp_filename); \
        __kv_blk_clear(&kv->block); \
        __kv_blk_clear(&tmp_blk); \
        sodium_memzero(&st, sizeof (st)); \
        sodium_memzero(&st_out, sizeof (st_out)); \
        sodium_munlock(&st, sizeof (st)); \
        sodium_munlock(&st_out, sizeof (st_out)); \
        sodium_munlock(tmp_blk.buffer, sizeof (tmp_blk.buffer)); \
    } while (false)

    uint8_t tmp_blk_buf[KV_BLOCK_SIZE] = {0};
    size_t bytes_read = fread(tmp_blk_buf, 1, sizeof (tmp_blk_buf), file);
    if (bytes_read != sizeof (tmp_blk_buf)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }
    size_t bytes_written = fwrite(tmp_blk_buf, 1, sizeof (tmp_blk_buf), file_out);
    if (bytes_written != sizeof (tmp_blk_buf)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }

    bytes_read = fread(header_buffer, 1, sizeof (header_buffer), file);
    if (bytes_read != sizeof (header_buffer)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header_buffer, kv->key) != 0) {
        ERROR_EXIT();
        return __kv_gen_error(CRYPTO_ERROR);
    }
    
    sodium_memzero(header_buffer, sizeof (header_buffer));

    if (crypto_secretstream_xchacha20poly1305_init_push(&st_out, header_buffer, kv->key) != 0) {
        ERROR_EXIT();
        return __kv_gen_error(CRYPTO_ERROR);
    }
    bytes_written = fwrite(header_buffer, 1, sizeof (header_buffer), file_out);
    if (bytes_written != sizeof (header_buffer)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }

    kv_item_t item_to_write = (kv_item_t){.key=key, .value=value, .timestamp=current_time};

    uint8_t ct_buffer[KV_CIPHERTEXT_BLOCK_SIZE] = {0};
    bool item_set = false;
    while (true) {
        size_t bytes_read = fread(ct_buffer, 1, sizeof (ct_buffer), file);
        if (bytes_read != sizeof (ct_buffer)) {
            // This is a read failure or a malformed file
            ERROR_EXIT();
            return __kv_gen_error(errno);
        }

        __kv_blk_clear(&kv->block);

        unsigned char tag;
        if (crypto_secretstream_xchacha20poly1305_pull(&st, kv->block.buffer, NULL, &tag, ct_buffer, sizeof (ct_buffer), NULL, 0) != 0) {
            ERROR_EXIT();
            return __kv_gen_error(CRYPTO_ERROR);
        }
   
        __kv_blk_set_filled(&kv->block);

        while (true) {
            result_opt_item_t maybe_opt_item = __kv_blk_consume_next_item(&kv->block);
            if (maybe_opt_item.is_error) {
                ERROR_EXIT();
                return result_opt_item_unwrap_err(maybe_opt_item);
            }

            optional_item_t maybe_item = result_opt_item_unwrap(maybe_opt_item);
            if (maybe_item.is_available) {
                kv_item_t item = optional_item_unwrap(maybe_item);
                if (!__kv_key_matches(key, item.key)) {
                    // Only <= the number of items are being written to a block from a block
                    // It should therefore be impossible to over run the size of the producing block,
                    // so no need to check size here, it is also checked in the produce_next function
                    kv_error_t err = __kv_blk_produce_next_item(&tmp_blk, item);
                    if (err.is_error) {
                        ERROR_EXIT();
                        return err;
                    }
                }
            } else if (!item_set) {
                // This is the end of the block
                size_t write_space_remaining = __kv_blk_producer_space_remaining(&tmp_blk);
                if (write_space_remaining >= __kv_item_total_size(item_to_write)) {
                    kv_error_t err = __kv_blk_produce_next_item(&tmp_blk, item_to_write);
                    if (err.is_error) {
                        ERROR_EXIT();
                        return err;
                    }
                    item_set = true;
                    break;
                }
            } else {
                break;
            }
        }

        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL || feof(file)) {
            // This is the end of the block and file
            if (item_set) {
                sodium_memzero(ct_buffer, sizeof (ct_buffer));
                __kv_blk_producer_pad_remaining(&tmp_blk);
                if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, tmp_blk.buffer, sizeof (tmp_blk.buffer), NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL) != 0) {
                    ERROR_EXIT();
                    return __kv_gen_error(CRYPTO_ERROR);
                }
                bytes_written = fwrite(ct_buffer, 1, sizeof (ct_buffer), file_out);
                if (bytes_written != sizeof (ct_buffer)) {
                    ERROR_EXIT();
                    return __kv_gen_error(errno);
                }
                sodium_memzero(ct_buffer, sizeof (ct_buffer));
                break;
            } else {
                sodium_memzero(ct_buffer, sizeof (ct_buffer));
                __kv_blk_producer_pad_remaining(&tmp_blk);
                if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, tmp_blk.buffer, sizeof (tmp_blk.buffer), NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_PUSH) != 0) {
                    ERROR_EXIT();
                    return __kv_gen_error(CRYPTO_ERROR);
                }
                bytes_written = fwrite(ct_buffer, 1, sizeof (ct_buffer), file_out);
                if (bytes_written != sizeof (ct_buffer)) {
                    ERROR_EXIT();
                    return __kv_gen_error(errno);
                }
                sodium_memzero(ct_buffer, sizeof (ct_buffer));
                __kv_blk_clear(&tmp_blk);
                kv_error_t err = __kv_blk_produce_next_item(&tmp_blk, item_to_write);
                if (err.is_error) {
                    ERROR_EXIT();
                    return err;
                }
                __kv_blk_producer_pad_remaining(&tmp_blk);
                if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, tmp_blk.buffer, sizeof (tmp_blk.buffer), NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL) != 0) {
                    ERROR_EXIT();
                    return __kv_gen_error(CRYPTO_ERROR);
                }
                bytes_written = fwrite(ct_buffer, 1, sizeof (ct_buffer), file_out);
                if (bytes_written != sizeof (ct_buffer)) {
                    ERROR_EXIT();
                    return __kv_gen_error(errno);
                }
                sodium_memzero(ct_buffer, sizeof (ct_buffer));
                break;
            }
        } else {
            // Just the end of the block
            sodium_memzero(ct_buffer, sizeof (ct_buffer));
            __kv_blk_producer_pad_remaining(&tmp_blk);
            if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, tmp_blk.buffer, sizeof (tmp_blk.buffer), NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_PUSH) != 0) {
                ERROR_EXIT();
                return __kv_gen_error(CRYPTO_ERROR);
            }
            bytes_written = fwrite(ct_buffer, 1, sizeof (ct_buffer), file_out);
            if (bytes_written != sizeof (ct_buffer)) {
                ERROR_EXIT();
                return __kv_gen_error(errno);
            }
            sodium_memzero(ct_buffer, sizeof (ct_buffer));
        }
    }

    fclose(file);
    fclose(file_out);

    __kv_blk_clear(&kv->block);
    __kv_blk_clear(&tmp_blk);

    sodium_memzero(&st, sizeof (st));
    sodium_memzero(&st_out, sizeof (st_out));

    sodium_munlock(&st, sizeof (st));
    sodium_munlock(&st_out, sizeof (st_out));
    sodium_munlock(tmp_blk.buffer, sizeof (tmp_blk.buffer));

    if (remove(kv->main_filename) != 0) {
        remove(kv->tmp_filename);
        return __kv_gen_error(errno);
    }

    if (rename(kv->tmp_filename, kv->main_filename) != 0) {
        // This case should be handled in a special manner
        // No data loss has occured, but the data is in a different file location than expected
        return __kv_gen_error(RENAME_TMP_TO_MAIN_FAILED);
    }

    return __kv_gen_success();
    #undef ERROR_EXIT
}
kv_error_t kv_delete (kv_t* kv, slice_t key) {
    assert(kv->initialized);
    assert(!kv->reserved);

    if (!kv->unlocked) {
        return __kv_gen_error(KV_IS_LOCKED);
    }
    result_uint64_t maybe_time = __kv_get_time_ms();
    if (maybe_time.is_error) {
        return result_uint64_unwrap_err(maybe_time);
    }

    uint64_t current_time = result_uint64_unwrap(maybe_time);

    uint64_t duration = current_time - kv->unlock_time;
    if (duration >= kv->max_unlock_time) {
        kv_lock(kv);
        return __kv_gen_error(KV_IS_LOCKED);
    }

    crypto_secretstream_xchacha20poly1305_state st = {0};
    crypto_secretstream_xchacha20poly1305_state st_out = {0};
    uint8_t header_buffer[crypto_secretstream_xchacha20poly1305_HEADERBYTES] = {0};
    if (sodium_mlock(&st, sizeof (st)) != 0 || sodium_mlock(&st_out, sizeof (st_out)) != 0) {
        return __kv_gen_error(OUT_OF_MEMORY);
    }
    kv_block_t tmp_blk = {0};
    __kv_blk_clear(&tmp_blk);
    if (sodium_mlock(tmp_blk.buffer, sizeof (tmp_blk.buffer)) != 0) {
        return __kv_gen_error(OUT_OF_MEMORY);
    }

    FILE* file = fopen(kv->main_filename, "rb");
    if (file == NULL) {
        // Maybe use errno here instead
        return __kv_gen_error(errno);
    }
    FILE* file_out = fopen(kv->tmp_filename, "wb");
    if (file_out == NULL) {
        fclose(file);
        return __kv_gen_error(errno);
    }

    #define ERROR_EXIT() do {\
        fclose(file); \
        fclose(file_out); \
        remove(kv->tmp_filename); \
        __kv_blk_clear(&kv->block); \
        __kv_blk_clear(&tmp_blk); \
        sodium_memzero(&st, sizeof (st)); \
        sodium_memzero(&st_out, sizeof (st_out)); \
        sodium_munlock(&st, sizeof (st)); \
        sodium_munlock(&st_out, sizeof (st_out)); \
        sodium_munlock(tmp_blk.buffer, sizeof (tmp_blk.buffer)); \
    } while (false)

    uint8_t tmp_blk_buf[KV_BLOCK_SIZE] = {0};
    size_t bytes_read = fread(tmp_blk_buf, 1, sizeof (tmp_blk_buf), file);
    if (bytes_read != sizeof (tmp_blk_buf)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }
    size_t bytes_written = fwrite(tmp_blk_buf, 1, sizeof (tmp_blk_buf), file_out);
    if (bytes_written != sizeof (tmp_blk_buf)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }

    bytes_read = fread(header_buffer, 1, sizeof (header_buffer), file);
    if (bytes_read != sizeof (header_buffer)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header_buffer, kv->key) != 0) {
        ERROR_EXIT();
        return __kv_gen_error(CRYPTO_ERROR);
    }
    
    sodium_memzero(header_buffer, sizeof (header_buffer));

    if (crypto_secretstream_xchacha20poly1305_init_push(&st_out, header_buffer, kv->key) != 0) {
        ERROR_EXIT();
        return __kv_gen_error(CRYPTO_ERROR);
    }
    bytes_written = fwrite(header_buffer, 1, sizeof (header_buffer), file_out);
    if (bytes_written != sizeof (header_buffer)) {
        ERROR_EXIT();
        return __kv_gen_error(errno);
    }

    uint8_t ct_buffer[KV_CIPHERTEXT_BLOCK_SIZE] = {0};
    while (true) {
        size_t bytes_read = fread(ct_buffer, 1, sizeof (ct_buffer), file);
        if (bytes_read != sizeof (ct_buffer)) {
            // This is a read failure or a malformed file
            ERROR_EXIT();
            return __kv_gen_error(errno);
        }

        __kv_blk_clear(&kv->block);

        unsigned char tag;
        if (crypto_secretstream_xchacha20poly1305_pull(&st, kv->block.buffer, NULL, &tag, ct_buffer, sizeof (ct_buffer), NULL, 0) != 0) {
            ERROR_EXIT();
            return __kv_gen_error(CRYPTO_ERROR);
        }
   
        __kv_blk_set_filled(&kv->block);

        while (true) {
            result_opt_item_t maybe_opt_item = __kv_blk_consume_next_item(&kv->block);
            if (maybe_opt_item.is_error) {
                ERROR_EXIT();
                return result_opt_item_unwrap_err(maybe_opt_item);
            }

            optional_item_t maybe_item = result_opt_item_unwrap(maybe_opt_item);
            if (maybe_item.is_available) {
                kv_item_t item = optional_item_unwrap(maybe_item);
                if (!__kv_key_prefix_matches(key, item.key)) {
                    // Only <= the number of items are being written to a block from a block
                    // It should therefore be impossible to over run the size of the producing block,
                    // so no need to check size here, it is also checked in the produce_next function
                    kv_error_t err = __kv_blk_produce_next_item(&tmp_blk, item);
                    if (err.is_error) {
                        ERROR_EXIT();
                        return err;
                    }
                }
            } else {
                break;
            }
        }

        tag = (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL || feof(file) ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : crypto_secretstream_xchacha20poly1305_TAG_PUSH);
        // Just the end of the block
        sodium_memzero(ct_buffer, sizeof (ct_buffer));
        __kv_blk_producer_pad_remaining(&tmp_blk);
        if (crypto_secretstream_xchacha20poly1305_push(&st_out, ct_buffer, NULL, tmp_blk.buffer, sizeof (tmp_blk.buffer), NULL, 0, tag) != 0) {
            ERROR_EXIT();
            return __kv_gen_error(CRYPTO_ERROR);
        }
        bytes_written = fwrite(ct_buffer, 1, sizeof (ct_buffer), file_out);
        if (bytes_written != sizeof (ct_buffer)) {
            ERROR_EXIT();
            return __kv_gen_error(errno);
        }
        sodium_memzero(ct_buffer, sizeof (ct_buffer));
        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL || feof(file)) break;
    }

    fclose(file);
    fclose(file_out);

    __kv_blk_clear(&kv->block);
    __kv_blk_clear(&tmp_blk);

    sodium_memzero(&st, sizeof (st));
    sodium_memzero(&st_out, sizeof (st_out));

    sodium_munlock(&st, sizeof (st));
    sodium_munlock(&st_out, sizeof (st_out));
    sodium_munlock(tmp_blk.buffer, sizeof (tmp_blk.buffer));

    if (remove(kv->main_filename) != 0) {
        remove(kv->tmp_filename);
        return __kv_gen_error(errno);
    }

    if (rename(kv->tmp_filename, kv->main_filename) != 0) {
        // This case should be handled in a special manner
        // No data loss has occured, but the data is in a different file location than expected
        return __kv_gen_error(RENAME_TMP_TO_MAIN_FAILED);
    }

    return __kv_gen_success();
    #undef ERROR_EXIT
}

// kv_util_hash_password creates a symmetric encryption key by hashing a salt with a password using Argon2ID
result_slice_t kv_util_hash_password (slice_t key_dest, slice_t salt, slice_t password) {
    assert(salt.len == crypto_pwhash_argon2i_SALTBYTES);
    assert(key_dest.len == 0);
    assert(key_dest.cap >= KV_KEY_SIZE);

    if (crypto_pwhash(key_dest.ptr, KV_KEY_SIZE, (char*)password.ptr, password.len, salt.ptr, 
    crypto_pwhash_argon2id_OPSLIMIT_SENSITIVE, crypto_pwhash_argon2id_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        return result_slice_error(__kv_gen_error(CRYPTO_ERROR));
    }

    return result_slice_ok(s_produce_end(key_dest, KV_KEY_SIZE));
}

// kv_util_fill_rand fills the remaining space in a slice with random bytes and returns the updated slice
slice_t kv_util_fill_rand (slice_t dest) {
    size_t space_remaining = dest.cap - dest.len;
    randombytes_buf(dest.ptr + dest.len, space_remaining);
    return s_produce_end(dest, space_remaining);
}

// kv_util_rand_choice returns a random uint32 in the range [min, max)
uint32_t kv_util_rand_choice (uint32_t min, uint32_t max) {
    assert(max > min);
    uint32_t range = max - min;
    uint64_t rand = __kv_gen_reservation();
    return (rand % range) + min;
}