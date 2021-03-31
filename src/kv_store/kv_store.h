#ifndef PASSMNGR_KVSTORE_H_
#define PASSMNGR_KVSTORE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <result.h>
#include <common.h>

#include <sodium.h>

#define PLAINTEXT_CHUNK_SIZE  (2048)
#define CIPHERTEXT_CHUNK_SIZE (PLAINTEXT_CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES)
#define ENCRYPTED_KEY_LENGTH  (crypto_secretstream_xchacha20poly1305_KEYBYTES + crypto_secretstream_xchacha20poly1305_HEADERBYTES + crypto_secretstream_xchacha20poly1305_ABYTES)

#define ITEM_KEY_LENGTH_INTEGER_TYPE       uint64_t
#define ITEM_VALUE_LENGTH_INTEGER_TYPE     uint64_t
#define ITEM_TIMESTAMP_INTEGER_TYPE        uint64_t
#define ITEM_HEADER_SIZE                   (sizeof (ITEM_KEY_LENGTH_INTEGER_TYPE) + sizeof (ITEM_VALUE_LENGTH_INTEGER_TYPE) + sizeof (ITEM_TIMESTAMP_INTEGER_TYPE))
#define ITEM_MAX_CONTENT_SIZE              (PLAINTEXT_CHUNK_SIZE - ITEM_HEADER_SIZE)

// Default max unlock time is 5 minutes
#define DEFAULT_MAX_UNLOCK_TIME (5.0 * 60.0)
// Default key life is 1 year (then a rekey will be done automatically)
#define DEFAULT_KEY_LIFE        (1 * 365 * 24 * 60 * 60)
// Default number of writes before a rekey is 1024
#define DEFAULT_MAX_KEY_WRITES  (1024)

typedef enum kv_error {
    // Runtime 'issues' are included first these are expected problems that don't indicate that something is wrong
    ISSUE_KEY_NOT_FOUND,
    ISSUE_KEY_LENGTH_INVALID,
    ISSUE_VALUE_LENGTH_INVALID,

    // Runtime errors indicate things that could happen, but can't be recovered from (file errors, out of memory, etc)
    RUNTIME_OUT_OF_MEMORY,
    RUNTIME_FILE_MALFORMED,
    RUNTIME_ENCRYPTION_FAILED,
    RUNTIME_CLOCK_ERROR,

    FILE_READ_FAILED,
    FILE_WRITE_FAILED,
    FILE_OPEN_FAILED,
    FILE_SEEK_FAILED,
    FILE_REMOVE_FAILED,
    FILE_RENAME_FAILED,

    RUNTIME_IS_LOCKED,
    RUNTIME_NOT_OPEN,
    RUNTIME_ALREADY_OPEN,
    RUNTIME_ITEM_NOT_FOUND,
    RUNTIME_ITEM_TOO_LARGE,

    FATAL_MALFORMED_FILE,
    FATAL_OUT_OF_MEMORY,
    FATAL_ENCRYPTION_FAILURE,
} kv_error_t;

typedef struct kv_value {
    byte_t data[PLAINTEXT_CHUNK_SIZE];
    ITEM_VALUE_LENGTH_INTEGER_TYPE length;
} kv_value_t;

typedef struct kv_key {
    byte_t data[PLAINTEXT_CHUNK_SIZE];
    ITEM_KEY_LENGTH_INTEGER_TYPE length;
} kv_key_t;

typedef struct kv_item {
    kv_key_t key;
    kv_value_t value;
} kv_item_t;

typedef struct kv_item_header {
    ITEM_KEY_LENGTH_INTEGER_TYPE key_length;
    ITEM_VALUE_LENGTH_INTEGER_TYPE value_length;
    ITEM_TIMESTAMP_INTEGER_TYPE timestamp;
} kv_item_header_t;

typedef struct kv {
    bool initialized;
    const char* read_filename;
    const char* write_filename;
    FILE* file;

    unsigned long kv_start;
    bool kv_empty;

    byte_t master_key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    
    bool tmps_borrowed;
    
    double unlock_start_time_s;
    double max_unlock_time_s;
    bool unlocked;

    uint64_t read_length;
    uint64_t overflow_length;
} kv_t;

RESULT_EMPTY_TYPE(kv_empty_result_t, kv_error_t)
RESULT_TYPE(kv_value_result_t, kv_value_t, kv_error_t)
RESULT_TYPE(kv_header_result_t, kv_item_header_t, kv_error_t)

double kv_util_get_monotonic_time();
bool   kv_util_is_past_expiration(double start_time, double max_time);

kv_empty_result_t kv_init   (kv_t* kv, const char* const read_filename, const char* const write_filename);
kv_empty_result_t kv_create (kv_t* kv, const char * const password, size_t password_length);
kv_empty_result_t kv_open   (kv_t* kv, const char * const password, size_t password_length);
kv_value_result_t kv_get    (kv_t* kv, kv_key_t key);
kv_empty_result_t kv_set    (kv_t* kv, kv_item_t item);
// kv_empty_result_t kv_delete (kv_t* kv, kv_key_t key);
kv_empty_result_t kv_close  (kv_t* kv);

#endif