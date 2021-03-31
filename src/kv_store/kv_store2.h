#ifndef PASSMNGR_KVSTORE_H_
#define PASSMNGR_KVSTORE_H_

// Include integer and boolean types
#include <stdint.h>
#include <stdbool.h>

// Include macros to create result and optional types
#include <result.h>
#include <kv_store_errors.h>

// Include constants from encryption library
#include <sodium.h>

// Define constants
// File content constants
#define MASTER_SALT_LENGTH           (crypto_pwhash_argon2id_SALTBYTES)
#define MASTER_KEY_LENGTH            (crypto_secretstream_xchacha20poly1305_KEYBYTES)
#define CIPHERTEXT_HEADER_LENGTH     (crypto_secretstream_xchacha20poly1305_HEADERBYTES)
#define CIPHERTEXT_OVERHEAD_LENGTH   (crypto_secretstream_xchacha20poly1305_ABYTES)

#define PLAINTEXT_CHUNK_SIZE         (2048)
#define CIPHERTEXT_CHUNK_SIZE        (PLAINTEXT_CHUNK_SIZE + CIPHERTEXT_OVERHEAD_LENGTH)

#define MASTER_KEY_CIPHERTEXT_LENGTH (MASTER_KEY_LENGTH + CIPHERTEXT_OVERHEAD_LENGTH)
#define MASTER_KEY_HEADER_LENGTH     (MASTER_SALT_LENGTH + CIPHERTEXT_HEADER_LENGTH + MASTER_KEY_CIPHERTEXT_LENGTH)

#define ITEM_KEY_LENGTH_TYPE         uint16_t
#define ITEM_VALUE_LENGTH_TYPE       uint16_t
#define ITEM_TIMESTAMP_TYPE          uint64_t

#define MIN_KEY_LENGTH               (1)
#define MIN_VALUE_LENGTH             (1)
#define MAX_KEY_LENGTH               (PLAINTEXT_CHUNK_SIZE - (MIN_VALUE_LENGTH + sizeof (ITEM_KEY_LENGTH_TYPE) + sizeof (ITEM_VALUE_LENGTH_TYPE) + sizeof (ITEM_TIMESTAMP_TYPE)))
#define MAX_VALUE_LENGTH             (PLAINTEXT_CHUNK_SIZE - (MIN_KEY_LENGTH + sizeof (ITEM_KEY_LENGTH_TYPE) + sizeof (ITEM_VALUE_LENGTH_TYPE) + sizeof (ITEM_TIMESTAMP_TYPE)))

// This is an arbitrary restriction to allow storing filenames in the kv struct without allocation


// This is an arbitrary restriction to allow storing error strings in the error struct without allocation
#define MAX_ERROR_STRING_LENGTH      (512)

// Define default parameters
#define FILE_STRLEN(S)               (sizeof (S) - 1) // Don't count the null terminator

#define DEFAULT_FILENAME_PREFIX      ((char[]){"vault."})
#define FILENAME_PREFIX_LENGTH       FILE_STRLEN(DEFAULT_FILENAME_PREFIX)
#define DEFAULT_FILE_EXTENSION       ((char[]){".ekv"})
#define FILE_EXTENSION_LENGTH        FILE_STRLEN(DEFAULT_FILE_EXTENSION)
#define DEFAULT_FILE_COUNTER_TYPE    uint64_t
#define DEFAULT_FILE_COUNTER_LENGTH  (2 * sizeof (DEFAULT_FILE_COUNTER_TYPE))
#define DEFAULT_FILENAME_LENGTH      (FILENAME_PREFIX_LENGTH + DEFAULT_FILE_COUNTER_LENGTH + FILE_EXTENSION_LENGTH)

#define MAX_PATH_LENGTH              (1024)
#define PATH_BUFFER_SIZE             (MAX_PATH_LENGTH + 1)
#define MAX_DIRECTORY_LENGTH         (MAX_PATH_LENGTH - DEFAULT_FILENAME_LENGTH)
#define DIRECTORY_BUFFER_SIZE        (MAX_DIRECTORY_LENGTH + 1)

typedef enum kv_state {
    UNINITIALIZED,
    BUFFERS_INITIALIZED,
    UNLOCKED,
    LOCKED
} kv_state_t;

typedef struct kv {
    kv_state_t state;                          // The current state of the kv

    char directory[MAX_DIRECTORY_LENGTH]; // The name of the directory where files are stored
    size_t directory_length;

    uint8_t master_key[MASTER_KEY_LENGTH];     // The key that encrypts and decrypts the KV
    
    bool buffers_borrowed;
    uint8_t value_buffer[MAX_VALUE_LENGTH];

    double unlock_time;                        // The timestamp when the master key was retrieved
    double max_unlock_duration;                // The maximum time that the key should be held in memory
} kv_t;

typedef struct kv_value {
    uint8_t (*value)[MAX_VALUE_LENGTH];
    ITEM_VALUE_LENGTH_TYPE length;
} kv_value_t;

typedef struct kv_key {
    uint8_t (*key)[MAX_KEY_LENGTH];
    ITEM_KEY_LENGTH_TYPE length;
} kv_key_t;

OPTIONAL_TYPE(kv_opt_value_t, kv_value_t);
RESULT_EMPTY_TYPE(kv_result_t, kv_error_t);
RESULT_TYPE(kv_result_opt_value_t, kv_opt_value_t, kv_error_t);

kv_result_t kv_initialize (kv_t* kv, const char* const directory);
kv_result_t kv_create     (kv_t* kv, const char* const password, size_t password_length);
kv_result_t kv_unlock     (kv_t* kv, const char* const password, size_t password_length);

kv_result_opt_value_t kv_get    (kv_t* kv, kv_key_t key);
kv_result_t           kv_set    (kv_t* kv, kv_key_t key, kv_value_t value);
kv_result_t           kv_delete (kv_t* kv, kv_key_t key);

kv_result_opt_value_t kv_borrow_value  (kv_t* kv);
void                  kv_release_value (kv_t* kv);


