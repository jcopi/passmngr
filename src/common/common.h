#ifndef PASSMNGR_COMMON_H_
#define PASSMNGR_COMMON_H_

#include <stdint.h>
#include <assert.h>

#define COMMON_ARCHIVE_ERROR_START  (0x000)
#define COMMON_VAULT_ERROR_START    (0x100)
#define COMMON_PASSWORD_ERROR_START (0x200)

#define COMMON_ITEM_NAME_TYPE uint16_t
#define COMMON_KEY_NAME_TYPE  uint16_t

typedef uint8_t byte_t;
static_assert(sizeof (byte_t) == 1, "Invalid byte size");

typedef enum file_mode {
    READ,
    WRITE
} file_mode_t;

#endif
