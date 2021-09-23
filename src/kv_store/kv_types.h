#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <slice.h>
#include <kv_constants.h>

typedef struct kv_item {
    slice_t key;
    slice_t value;
    uint64_t timestamp;
} kv_item_t;

typedef struct kv_item_header {
    ITEM_KEY_LENGTH_TYPE key_length;
    ITEM_VALUE_LENGTH_TYPE value_length;
    ITEM_TIMESTAMP_TYPE timestamp;
} kv_item_header_t;

typedef struct kv_error {
    bool is_error;
    int32_t code;
} kv_error_t;

typedef struct kv_block {
    uint8_t buffer[KV_BLOCK_SIZE];
    size_t bytes_produced;
    size_t bytes_consumed;
} kv_block_t;

typedef struct kv {
    // KV state
    bool unlocked;
    bool initialized;
    bool reserved;

    uint64_t reservation;
    uint64_t unlock_time;

    // KV Settings
    uint64_t max_unlock_time;

    // Sensitive memory regions
    uint8_t key[KV_KEY_SIZE];
    kv_block_t block;

    // Non-sensitive memory regions
    char main_filename[MAX_PATH_LENGTH + 1];
    char tmp_filename[MAX_PATH_LENGTH + 1];
} kv_t;

enum kv_error_codes {
    NO_ERROR = 0,
    INVALID_RESERVATION = -1,
    OUT_OF_MEMORY = -2,
    FILE_PATH_TOO_LONG = -3,
    MALFORMED_BLOCK = -4,
    FAILED_TO_GET_TIME = -5,
    KV_IS_LOCKED = -7,
    CRYPTO_ERROR = -8,
    MALFORMED_FILE = -9,
    ITEM_TOO_LARGE = -10,
    RENAME_TMP_TO_MAIN_FAILED = -11,
};