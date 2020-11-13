#include <vault.h>
#include <sodium.h>

#define VAULT_SALT_ITEM_NAME "salt"
#define VAULT_SALT_NAME_BYTES (4)
#define VAULT_KEYS_ITEM_NAME "keys"
#define VAULT_KEYS_NAME_BYTES (4)

vault_item_result_t vault_open_item_with_key(vault_t* v, byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes, byte_t* key);
vault_empty_result_t vault_parse_keys(vault_item_t* vi);
void vault_clear_keys(vault_t* v);

vault_result_t vault_open (const char* file_name, vault_mode_t mode)
{
    vault_t vault = {0};
    vault_result_t result = {0};

    archive_result_t maybe_archive = archive_open(file_name, mode);
    if (IS_ERR(maybe_archive)) {
        // Handle the error
        return ERR(result, UNWRAP_ERR(maybe_archive));
    }

    if (sodium_init() < 0) {
        return ERR(result, VAULT_CRYPTO_LIB_ERROR);
    }

    vault.archive = UNWRAP(maybe_archive);

    switch (mode) {
    case READ:
        // Verify the existance of necessary items:
        //  - salt: The salt used of password hashing
        //  - keys: The file of keys for other items
        bool has_salt = archive_has_item(&vault.archive, VAULT_SALT_ITEM_NAME, VAULT_SALT_NAME_BYTES);
        bool has_keys = archive_has_item(&vault.archive, VAULT_KEYS_ITEM_NAME, VAULT_KEYS_NAME_BYTES);
        
        if (!has_salt) {
            return ERR(result, VAULT_MISSING_SALT);
        }

        if (!has_keys) {
            return ERR(result, VAULT_MISSING_KEYS);
        }

        vault.opened = true;

        return OK(result, vault);
    case WRITE:
        return OK(result, vault);
    default: return ERR(result, VAULT_UNKNOWN_MODE);
    }
}

vault_empty_result_t vault_unlock (vault_t* v, byte_t* password, size_t password_bytes)
{
    assert(v->opened);
    assert(v->locked);
    assert(!v->borrowed);

    vault_empty_result_t result = {0};

    byte_t* master_key = NULL;
    byte_t master_salt[crypto_pwhash_SALTBYTES];

    archive_item_result_t maybe_item = archive_item_open(&v->archive, VAULT_SALT_ITEM_NAME, VAULT_SALT_NAME_BYTES);
    if (IS_ERR(maybe_item)) {
        // Handle error
        ERR(result, VAULT_MISSING_SALT);
    }
    archive_item_t salt_item = UNWRAP(maybe_item);
    archive_size_result_t maybe_size = archive_item_read(&salt_item, master_salt, crypto_pwhash_SALTBYTES);
    if (IS_ERR(maybe_size)) {
        ERR(result, UNWRAP_ERR(maybe_size))
    }
    if (UNWRAP(maybe_size) != crypto_pwhash_SALTBYTES) {
        ERR(result, VAULT_INVALID_SALT)
    }

    master_key = sodium_allocarray(crypto_secretstream_xchacha20poly1305_KEYBYTES, 1);
    if (master_key == NULL) {
        sodium_memzero(master_salt);
        return ERR(result, VAULT_OUT_OF_MEMORY);
    }
    if (sodium_mprotect_readwrite(master_key) != 0) {
        sodium_free(master_key);
        return ERR(result, VAULT_MEMORY_PROTECTION_FAILED)
    }

    if (crypto_pwhash(master_key, crypto_secretstream_xchacha20poly1305_KEYBYTES, password, password_bytes, master_salt,
        crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        sodium_free(master_key);
        return ERR(result, VAULT_ERROR_HASHING_PASSWORD);
    }
    if (sodium_mprotect_readonly(master_key) != 0) {
        sodium_free(master_key);
        return ERR(result, VAULT_MEMORY_PROTECTION_FAILED)
    }

    vault_item_result_t maybe_vault_item = vault_open_item_with_key(&v, VAULT_KEY_ITEM_NAME, VAULT_KEYS_NAME_BYTES, master_key);
    if (IS_ERR(maybe_vault_item)) {
        sodium_free(master_key);
        return ERR(result, UNWRAP_ERR(maybe_vault_item));
    }
    vault_item_t vault_item = UNWRAP(maybe_vault_item);

    vault_empty_result_t maybe_vault_empty = vault_parse_keys(&vault_item);
    if (IS_ERR(maybe_vault_empty)) {
        sodium_free(master_key);
        return ERR(result, UNWRAP_ERR(maybe_vault_empty));
    }

    maybe_vault_empty = vault_close_item(&vault_item);
    if (IS_ERR(maybe_vault_empty)) {
        sodium_free(master_key);
        return ERR(result, UNWRAP_ERR(maybe_vault_empty));
    }

    vault->unlocked = true;
    
    return OK(result);
}

vault_empty_result_t vault_init (vault_t* v, byte_t* password, size_t password_bytes) {
    
}

vault_empty_result_t vault_lock   (vault_t* v);
vault_empty_result_t vault_close  (vault_t* v);

vault_item_result_t  vault_open_item  (vault_t* v, byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes)
{
    assert(v->opened);
    assert(!v->locked);
    assert(!v->borrowed);

    size_t i;
    for (i = 0; i < key_count; i++) {
        
    }
}
vault_size_result_t  vault_write_item (vault_item_t* vi, const byte_t* buffer, size_t buffer_bytes);
vault_size_result_t  vault_read_item  (vault_item_t* vi, byte_t* buffer, size_t buffer_bytes);
vault_empty_result_t vault_close_item (vault_item_t* vi);

vault_result_t vault_duplicate_except (vault_t* v, byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes);
vault_result_t vault_duplicate_rekey  (vault_t* v);
