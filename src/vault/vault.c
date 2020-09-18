#include <vault.h>

vault_result_t vault_open (const char* file_name, vault_mode_t mode)
{
    vault_t vault = {0};
    vault_result_t result = {0};

    archive_result_t maybe_archive = archive_open(file_name, mode);
    if (IS_ERR(maybe_archive)) {
        // Handle the error
        return ERR(result, 0);
    }

    

    return OK(result, vault);
}

vault_empty_result_t vault_unlock (vault_t* v, byte_t* password, size_t password_bytes);
vault_empty_result_t vault_lock   (vault_t* v);
vault_empty_result_t vault_close  (vault_t* v);

vault_item_result_t  vault_open_item  (vault_t* v, byte_t* name, uint16_t name_bytes);
vault_size_result_t  vault_write_item (vault_item_t* vi, const byte_t* buffer, size_t buffer_bytes);
vault_size_result_t  vault_read_item  (vault_item_t* vi, byte_t* buffer, size_t buffer_bytes);
vault_empty_result_t vault_close_item (vault_item_t* vi);

vault_result_t vault_duplicate_except (vault_t* v, byte_t* name, uint16_t name_bytes);
vault_result_t vault_duplicate_rekey  (vault_t* v);