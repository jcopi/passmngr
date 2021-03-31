#ifndef PASSMNGR_KVSTORE_ERRORS_H_
#define PASSMNGR_KVSTORE_ERRORS_H_

typedef enum kv_error_code {
    OUT_OF_MEMORY,
    CLOCK_READ_FAILED,
    FILE_OPERATION_FAILED,
    ENCRYPTION_OPERATION_FAILED,
    KDF_OPERATION_FAILED
} kv_error_code_t;

typedef struct kv_error {
    kv_error_code_t error_code;
    char* error_string;
} kv_error_t;

#define KV_ERROR(C,S) (kv_error_t){.error_code=C, .error_string=S}

const char* const KV_ERROR_STRINGS[] = {
    "Failed to initialize libsodium",
    "Failed to mlock critical memory",
    "Failed to generate key for password",
    "Failed to perform crypto operation"
};

#define KV_ERR_SODIUM_INIT (KV_ERROR_STRINGS[0])
#define KV_ERR_MLOCK       (KV_ERROR_STRINGS[1])
#define KV_ERR_KDF         (KV_ERROR_STRINGS[2])
#define KV_ERR_CRYPTO      (KV_ERROR_STRINGS[3])