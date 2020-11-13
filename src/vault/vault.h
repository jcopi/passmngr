#ifndef PASSMNGR_VAULT_H_
#define PASSMNGR_VAULT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <threads.h>

#include <result.h>
#include <common.h>
#include <archive.h>

typedef enum vault_mode {
    READ,
    WRITE,
} vault_mode_t;

typedef enum vault_error {
    AUTH_INVALID_PASSWORD = COMMON_VAULT_ERROR_START,
    VAULT_MISSING_SALT,
    VAULT_INVALID_SALT,
    VAULT_MISSING_KEYS,
    VAULT_ERROR_HASHING_PASSWORD,
    VAULT_OUT_OF_MEMORY,
    VAULT_MEMORY_PROTECTION_FAILED,
    VAULT_CRYPTO_LIB_ERROR,
    VAULT_UNKNOWN_MODE
} vault_error_t;

#define VAULT_AEAD_KEY_BYTES (64)

typedef struct vault_key {
    byte_t* key_content;
    size_t key_bytes;
} vault_key_t;

typedef enum vault_key_type {
    VAULT_SSH_PRIVATE_KEY,
    VAULT_SSH_PUBLIC_KEY,
    VAULT_AEAD_KEY,
    VAULT_PUBLIC_KEY,
    VAULT_PRIVATE_KEY
} vault_key_type_t;

typedef struct vault_key_usage {
    vault_key_type_t     type;
    COMMON_KEY_NAME_TYPE name_bytes;
    byte_t*              name_content;
    
} vault_key_usage_t;

typedef struct vault_slice {
    byte_t* content;
    size_t  bytes;
} vault_slice_t;

typedef struct vault {
    archive_t archive;

    bool borrowed;
    bool locked;
    bool opened;

    vault_key_t* keys;
    vault_key_usages_t* usages;
    size_t key_count;

    uint64_t max_unlocked_time;
    uint64_t unlock_time;
    atomic_uint_least64_t current_time;

    thrd_t timer_thread;
} vault_t;

typedef struct vault_item {
    vault_t* parent;
    size_t item_index;
    
    crypto_secretstream_xchacha20poly1305_state* state;
    byte_t* plaintext_buffer;
    size_t plaintext_index;
    size_t plaintext_bytes;
} vault_item_t;

RESULT_EMPTY_TYPE(vault_empty_result_t, vault_error_t)
RESULT_TYPE(vault_result_t, vault_t, vault_error_t)
RESULT_TYPE(vaule_item_result_t, vault_item_t, vault_error_t)
RESULT_TYPE(vault_size_result_t, size_t, vault_error_t)

vault_result_t       vault_open   (const char* file_name, vault_mode_t mode);
vault_empty_result_t vault_unlock (vault_t* v, byte_t* password, size_t password_bytes);
vault_empty_result_t vault_lock   (vault_t* v);
vault_empty_result_t vault_close  (vault_t* v);

vault_item_result_t  vault_open_item  (vault_t* v, byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes);
vault_size_result_t  vault_write_item (vault_item_t* vi, const byte_t* buffer, size_t buffer_bytes);
vault_size_result_t  vault_read_item  (vault_item_t* vi, byte_t* buffer, size_t buffer_bytes);
vault_empty_result_t vault_close_item (vault_item_t* vi);

vault_result_t vault_duplicate_except (vault_t* v, byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes);
vault_result_t vault_duplicate_rekey  (vault_t* v);

#endif
