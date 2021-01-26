#ifndef PASSMNGR_KV_STORE_
#define PASSMNGR_KV_STORE_

#include <stdint.h>
#include <stdbool.h>

#include <result.h>
#include <common.h>
#include <archive.h>

typedef enum kv_error {

} kv_error_t;

typedef struct kv_value {
    byte_t* value;
    size_t value_size;
} kv_value_t;

typedef struct kv_key {
    byte_t* key;
    size_t key_size;
} kv_key_t;

typedef struct kv_item {
    byte_t* key;
    size_t key_size;
    kv_value_t value;
} kv_item_t;

typedef struct kv {
    archive_t archive;

    bool borrowed;
    bool locked;
    bool opened;

    byte_t* ciphertext_buffer;
    byte_t* plaintext_buffer;

    size_t cipertext_size;
    size_t plaintext_size;
} kv_t;

RESULT_TYPE(kv_result_t, kv_t, kv_error_t);
RESULT_TYPE(kv_value_result_t, kv_value_t, kv_error_t);
RESULT_EMPTY_TYPE(kv_empty_result_t, kv_error_t);

kv_result_t       kv_open   (const char* file_name, file_mode_t mode);
kv_empty_result_t kv_unlock (const byte_t* password, size_t passowrd_size);
kv_value_result_t kv_get    (kv_key_t key);
kv_empty_result_t kv_set    (kv_key_t key, kv_value_t value);

#endif