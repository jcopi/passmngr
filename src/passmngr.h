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

#define MAX_FIELD_LENGTH (ITEM_MAX_CONTENT_SIZE / 2)

typedef enum pm_error {
    USER_PASSWORD_TOO_LONG,
    GENERATED_PASSWORD_TOO_LONG,
} pm_error_t;

typedef struct passmngr {
    kv_t kv;

    
} passmngr_t;

typedef struct pm {
    kv_t kv;
} pm_t;

typedef struct account {
    byte_t username[MAX_FIELD_LENGTH];
    byte_t password[MAX_FIELD_LENGTH];
    byte_t domain_name[MAX_FIELD_LENGTH];
} pm_account_t;

/*
    Open (Vault Filename)
    Unlock (Password) *Read password from stdin
    Lock ()
    Show Account (Domain) *Password shown
    Show All Accounts () *No passwords shown
    Add Account Generate Password (domain, username, Password Generation Details)
    Add Account Manual Password (domain, username, password)
    Set Account Detail (Domain, item name, item value)
    Delete Account (Domain)
    Get Remaining Unlock Time ()
    Change Master Password (New Password)
    Rekey vault
*/

RESULT_EMPTY_TYPE(pm_empty_result_t, pm_error_t)
RESULT_TYPE(pm_account_result_t, pm_account_t, pm_error_t)

pm_empty_result_t open   (pm_t* pm, const char const * filename);
pm_empty_result_t unlock (pm_t* pm, const char const * password, size_t password_length);
pm_empty_result_t lock   (pm_t* pm);

pm_empty_result_t input_password (byte_t (*password_buffer)[MAX_PASSWORD_SIZE], size_t* password_length);

#endif
