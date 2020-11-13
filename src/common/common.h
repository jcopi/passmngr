#ifndef PASSMNGR_COMMON_H_
#define PASSMNGR_COMMON_H_

#include <stdint.h>
#include <sodium.h>

#define COMMON_ARCHIVE_ERROR_START  (0x000)
#define COMMON_VAULT_ERROR_START    (0x100)
#define COMMON_PASSWORD_ERROR_START (0x200)

#define COMMON_ITEM_NAME_TYPE uint16_t
#define COMMON_KEY_NAME_TYPE  uint16_t

#define COMMON_STREAM_PLAIN_CHUNK_BYTES  (0x1000) // Encrypted in 4kB Chunks (4096B)
#define COMMON_STREAM_CIPHER_CHUNK_BYTES (COMMON_STREAM_PLAIN_CHUNK_BYTES + crypto_secretstream_xchacha20poly1305_ABYTES)

typedef uint8_t byte_t;
static_assert(sizeof (byte_t) == 1, "Invalid byte size");

typedef enum file_mode {
    READ,
    WRITE
} file_mode_t;

#endif
