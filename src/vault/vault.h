#ifndef PASSMNGR_VAULT_H_
#define PASSMNGR_VAULT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#include <result.h>
#include <archive.h>

typedef enum vault_mode {
    READ,
    WRITE,
} vault_mode_t;

typedef enum vault_error {

} vault_error_t;

typedef struct vault_aead_key {
    byte_t bytes[64];
} vault_aead_key_t;

typedef struct vault_slice {
    byte_t* content;
    size_t  bytes;
} vault_slice_t;

typedef struct vault {
    archive_t archive;

    bool borrowed;
    bool locked;
    bool opened;

    vault_aead_key_t* keys;
    vault_slice_t* usages;
    size_t key_count;

    uint32_t max_unlocked_time;
    uint32_t unlock_time;
    uint32_t current_time;

    
} vault_t;

typedef struct vault_item {
    vault_t* parent;
    size_t item_index;
} vault_item_t;

RESULT_EMPTY_TYPE(vault_empty_result_t, vault_error_t)
RESULT_TYPE(vault_result_t, vault_t, vault_error_t)
RESULT_TYPE(vaule_item_result_t, vault_item_t, vault_error_t)
RESULT_TYPE(vault_size_result_t, size_t, vault_error_t)

typedef struct vault_result {
    bool is_ok;
    union {
        vault_t value;
        vault_error_t error;
    } result;
} vault_result_t;

typedef struct vault_item_result {
    bool is_ok;
    union {
        vault_item_t value;
        vault_error_t error;
    } result;
} vault_item_result_t;

vault_result_t       vault_open   (const char* file_name, vault_mode_t mode);
vault_empty_result_t vault_unlock (vault_t* v, byte_t* password, size_t password_bytes);
vault_empty_result_t vault_close  (vault_t* v);

vault_item_result_t  vault_open_item  (vault_t* v, byte_t* name, uint16_t name_bytes);
vault_size_result_t  vault_write_item (vault_item_t* vi, const byte_t* buffer, size_t buffer_bytes);
vault_size_result_t  vault_read_item  (vault_item_t* vi, byte_t* buffer, size_t buffer_bytes);
vault_empty_result_t vault_close_item (vault_item_t* vi);

vault_result_t vault_duplicate_except (vault_t* v, byte_t* name, uint16_t name_bytes);
vault_result_t vault_duplicate_rekey  (vault_t* v);

#endif