#ifndef __PASSMNGR_H
#define __PASSMNGR_H

#include <common.h>
#include <result.h>
#include <kv_store.h>
#include <wordlist.h>

// An arbitary restriction to allow for no memory allocation
#define MAX_PASSWORD_SIZE       (4096)
// Default max unlock time is 5 minutes
#define DEFAULT_MAX_UNLOCK_TIME (5.0 * 60.0)
// Default key life is 1 year (then a rekey will be done automatically)
#define DEFAULT_KEY_LIFE        (1 * 365 * 24 * 60 * 60)

typedef enum pm_error {
    USER_PASSWORD_TOO_LONG,
    GENERATED_PASSWORD_TOO_LONG,
} pm_error_t;

RESULT_EMPTY_TYPE(pm_empty_result_t, pm_error_t)

typedef struct pm {
    kv_t kv;

    double unlock_time;
    double max_unlock_time;
} pm_t;

pm_empty_result_t input_password (byte_t (*password_buffer)[MAX_PASSWORD_SIZE], size_t* password_length);

#endif
