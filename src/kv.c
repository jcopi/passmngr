#include <kv.h>
#include <sqlite3.h>
#include <sodium.h>
#include <string.h>

#define PADDING_BLOCK_SIZE (16)

const char* const metatable_creation_query =
"CREATE TABLE IF NOT EXISTS metadata ("
"    key   TEXT UNIQUE PRIMARY KEY,"
"    value BLOB"
");";
const char* const itemtable_creation_query = 
"CREATE TABLE IF NOT EXISTS items ("
"    key_hash  BLOB UNIQUE PRIMARY KEY,"
"    key_nonce BLOB UNIQUE,"
"    key_bytes BLOB,"
"    val_nonce BLOB UNIQUE,"
"    val_bytes BLOB"
");";

const char* const get_value_query = 
"SELECT val_nonce, val_bytes FROM items WHERE key_hash = ?;";

const char* const set_value_query = 
"INSERT INTO items(key_hash, key_nonce, key_bytes, val_nonce, val_bytes)"
"  VALUES(?, ?, ?, ?, ?)"
"  ON CONFLICT(key_hash) DO UPDATE SET"
"    key_nonce=excluded.key_nonce,"
"    key_bytes=excluded.key_bytes,"
"    val_nonce=excluded.val_nonce,"
"    val_bytes=excluded.val_bytes;";

const char* const del_value_query = 
"DELETE FROM items WHERE key_hash = ?;";

const char* const get_metadata_query =
"SELECT value FROM metadata WHERE key = ?;";

const char* const value_iter_query = 
"SELECT key_nonce, key_bytes, val_nonce, val_bytes FROM items;";

const char* const encrypt_salt_name = "encryption_salt";
const char* const signing_salt_name = "signing_salt";
const char* const set_metadata_query = 
"INSERT INTO metadata(key, value)"
"  VALUES(?, ?)"
"  ON CONFLICT(key) DO UPDATE SET"
"    value=excluded.value;";

slice_result_t alloc_slice (size_t capacity);
slice free_slice(slice s);

nil_result_t init_vault () {
    if (sodium_init() < 0) {
        return ERROR(nil_result_t, -1);
    }

    return NIL(nil_result_t);
}

vault_result_t new_vault (const char* const db_file, slice password) {
    vault_t v = {0};
    vault_result_t result = {0};

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    guarded_t enc_key = {0};
    guarded_t sig_key = {0};

    //uint8_t* enc_buf = NULL;
    //uint8_t* sig_buf = NULL;

    uint8_t enc_salt[crypto_pwhash_argon2i_SALTBYTES] = {0};
    uint8_t sig_salt[crypto_pwhash_argon2i_SALTBYTES] = {0};

    v.db_file = db_file;
    
    int rc = sqlite3_open(v.db_file, &db);
    if (rc != SQLITE_OK) {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_prepare_v2(db, metatable_creation_query, strlen(metatable_creation_query), &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    rc = sqlite3_prepare_v2(db, itemtable_creation_query, strlen(itemtable_creation_query), &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    // Fetch the encryption key salt. If it does not exist create one and store it in the db
    rc = sqlite3_prepare_v2(db, get_metadata_query, strlen(get_metadata_query), &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_bind_text(stmt, 1, encrypt_salt_name, strlen(encrypt_salt_name), SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    bool need_to_set_salt = false;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        randombytes_buf(enc_salt, sizeof (enc_salt));
        need_to_set_salt = true;
    } else if (rc == SQLITE_ROW) {
        if (sqlite3_column_bytes(stmt, 0) != sizeof (enc_salt)) {
            result = ERROR(vault_result_t, -1);
            goto cleanup;
        }

        const uint8_t* ptr = sqlite3_column_blob(stmt, 0);
        if (ptr == NULL) {
            result = ERROR(vault_result_t, -1);
            goto cleanup;
        }

        memmove(enc_salt, ptr, sizeof (enc_salt));
    } else {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    if (need_to_set_salt) {
        rc = sqlite3_prepare_v2(db, set_metadata_query, strlen(set_metadata_query), &stmt, NULL);
        if (rc != SQLITE_OK) {
            result = ERROR(vault_result_t, rc);
            goto cleanup;
        }

        rc = sqlite3_bind_text(stmt, 1, encrypt_salt_name, strlen(encrypt_salt_name), SQLITE_STATIC);
        if (rc != SQLITE_OK) {
            result = ERROR(vault_result_t, rc);
            goto cleanup;
        }

        rc = sqlite3_bind_blob(stmt, 2, enc_salt, sizeof (enc_salt), SQLITE_STATIC);
        if (rc != SQLITE_OK) {
            result = ERROR(vault_result_t, rc);
            goto cleanup;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            result = ERROR(vault_result_t, rc);
            goto cleanup;
        }

        sqlite3_finalize(stmt);
        stmt = NULL;
    }

    // Fetch the signing key salt. If it does not exist create one and store it in the db
    rc = sqlite3_prepare_v2(db, get_metadata_query, strlen(get_metadata_query), &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_bind_text(stmt, 1, signing_salt_name, strlen(signing_salt_name), SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    need_to_set_salt = false;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        randombytes_buf(sig_salt, sizeof (sig_salt));
        need_to_set_salt = true;
    } else if (rc == SQLITE_ROW) {
        if (sqlite3_column_bytes(stmt, 0) != sizeof (sig_salt)) {
            result = ERROR(vault_result_t, -1);
            goto cleanup;
        }

        const uint8_t* ptr = sqlite3_column_blob(stmt, 0);
        if (ptr == NULL) {
            result = ERROR(vault_result_t, -1);
            goto cleanup;
        }

        memmove(sig_salt, ptr, sizeof (sig_salt));
    } else {
        result = ERROR(vault_result_t, rc);
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    if (need_to_set_salt) {
        rc = sqlite3_prepare_v2(db, set_metadata_query, strlen(set_metadata_query), &stmt, NULL);
        if (rc != SQLITE_OK) {
            result = ERROR(vault_result_t, rc);
            goto cleanup;
        }

        rc = sqlite3_bind_text(stmt, 1, signing_salt_name, strlen(signing_salt_name), SQLITE_STATIC);
        if (rc != SQLITE_OK) {
            result = ERROR(vault_result_t, rc);
            goto cleanup;
        }

        rc = sqlite3_bind_blob(stmt, 2, sig_salt, sizeof (sig_salt), SQLITE_STATIC);
        if (rc != SQLITE_OK) {
            result = ERROR(vault_result_t, rc);
            goto cleanup;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            result = ERROR(vault_result_t, rc);
            goto cleanup;
        }

        sqlite3_finalize(stmt);
        stmt = NULL;
    }

    guarded_result_t gr = sodium_alloc_slice(crypto_secretbox_KEYBYTES);
    if (IS_ERROR(gr)) {
        result = ERROR(vault_result_t, -1);
        goto cleanup;
    }
    enc_key = UNWRAP(gr);

    gr = sodium_alloc_slice(crypto_generichash_KEYBYTES);
    if (IS_ERROR(gr)) {
        result = ERROR(vault_result_t, -1);
        goto cleanup;
    }
    sig_key = UNWRAP(gr);

    nil_result_t nr = guarded_read_write(enc_key);
    if (IS_ERROR(nr)) {
        result = ERROR(vault_result_t, -1);
        goto cleanup;
    }
    nr = guarded_read_write(sig_key);
    if (IS_ERROR(nr)) {
        result = ERROR(vault_result_t, -1);
        goto cleanup;
    }

    if (crypto_pwhash(enc_key.ptr, enc_key.cap, password.ptr, password.len, enc_salt, crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        result = ERROR(vault_result_t, -1);
        goto cleanup;
    }
    enc_key = set_slice_len(enc_key, enc_key.cap);

    if (crypto_pwhash(sig_key.ptr, sig_key.cap, password.ptr, password.len, sig_salt, crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        result = ERROR(vault_result_t, -1);
        goto cleanup;
    }
    sig_key = set_slice_len(sig_key, sig_key.cap);

    v.encrypt_key = enc_key;
    v.signing_key = sig_key;

    result = VALUE(vault_result_t, v);
    goto success;

cleanup:
    sodium_free_slice(sig_key);
    sodium_free_slice(enc_key);
success:
    guarded_no_access(v.encrypt_key);
    guarded_no_access(v.signing_key);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

void destroy_vault (vault_t* v) {
    sodium_free(v->encrypt_key.ptr);
    sodium_free(v->signing_key.ptr);

    *v = (vault_t){0};
}

guarded_result_t vault_get (vault_t* v, slice key) {
    guarded_result_t result = {0};

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    guarded_t signature = {0};
    guarded_t value = {0};

    nil_result_t nr = guarded_read_only(v->encrypt_key);
    if (IS_ERROR(nr)) {
        result = ERROR(guarded_result_t, -1);
        goto cleanup;
    }
    nr = guarded_read_only(v->signing_key);
    if (IS_ERROR(nr)) {
        result = ERROR(guarded_result_t, -1);
        goto cleanup;
    }

    guarded_result_t gr = sodium_alloc_slice(crypto_generichash_BYTES);
    if (IS_ERROR(gr)) {
        result = ERROR(guarded_result_t, -1);
        goto cleanup;
    }
    signature = UNWRAP(gr);

    if (crypto_generichash(signature.ptr, signature.cap, key.ptr, key.len, v->signing_key.ptr, v->signing_key.len) != 0) {
        result = ERROR(guarded_result_t, -1);
        goto cleanup;
    }
    signature = set_slice_len(signature, signature.cap);

    int rc = sqlite3_open(v->db_file, &db);
    if (rc != SQLITE_OK) {
        result = ERROR(guarded_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_prepare_v2(db, get_value_query, strlen(get_value_query), &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = ERROR(guarded_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_bind_blob(stmt, 1, signature.ptr, signature.len, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(guarded_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        if (sqlite3_column_bytes(stmt, 0) != crypto_secretbox_NONCEBYTES) {
            result = ERROR(guarded_result_t, -1);
            goto cleanup;
        }

        const uint8_t* nonce = sqlite3_column_blob(stmt, 0);
        if (nonce == NULL) {
            result = ERROR(guarded_result_t, -1);
            goto cleanup;
        }

        size_t ct_size = sqlite3_column_bytes(stmt, 1);
        if (ct_size <= PADDING_BLOCK_SIZE) {
            result = ERROR(guarded_result_t, -1);
            goto cleanup;
        }

        const uint8_t* ct = sqlite3_column_blob(stmt, 1);
        if (ct == NULL) {
            result = ERROR(guarded_result_t, -1);
            goto cleanup;
        }

        gr = sodium_alloc_slice(ct_size - crypto_secretbox_MACBYTES);
        if (IS_ERROR(gr)) {
            result = ERROR(guarded_result_t, -1);
            goto cleanup;
        }
        value = UNWRAP(gr);

        if (crypto_secretbox_open_easy(value.ptr, ct, ct_size, nonce, v->encrypt_key.ptr) != 0) {
            result = ERROR(guarded_result_t, -1);
            goto cleanup;
        }

        sqlite3_finalize(stmt);
        stmt = NULL;

        size_t unpad_size = 0;
        if (sodium_unpad(&unpad_size, value.ptr, value.cap, PADDING_BLOCK_SIZE) != 0) {
            result = ERROR(guarded_result_t, -1);
            goto cleanup;
        }
        value = set_slice_len(value, unpad_size);
        nil_result_t nr = guarded_read_only(value);
        if (IS_ERROR(nr)) {
            result = ERROR(guarded_result_t, -1);
            goto cleanup;
        }

        result = VALUE(guarded_result_t, value);
        goto success;
    } else if (rc == SQLITE_DONE) {
        result = NIL(guarded_result_t);
        goto cleanup;
    } else {
        result = ERROR(guarded_result_t, rc);
        goto cleanup;
    }

cleanup:
    sodium_free_slice(value);
success:
    sodium_free_slice(signature);
    guarded_no_access(v->encrypt_key);
    guarded_no_access(v->signing_key);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

nil_result_t vault_iter (vault_t* v, slice prefix, void (*iter)(void* ctx, guarded_t key, guarded_t value), void* ctx) {
    nil_result_t result = {0};

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    guarded_t value = {0};
    guarded_t key = {0};

    nil_result_t nr = guarded_read_only(v->encrypt_key);
    if (IS_ERROR(nr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    nr = guarded_read_only(v->signing_key);
    if (IS_ERROR(nr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }

    int rc = sqlite3_open(v->db_file, &db);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_prepare_v2(db, value_iter_query, strlen(value_iter_query), &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
        key = sodium_free_slice(key);
        value = sodium_free_slice(value);

        // Decrypt Key
        if (sqlite3_column_bytes(stmt, 0) != crypto_secretbox_NONCEBYTES) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        const uint8_t* knonce = sqlite3_column_blob(stmt, 0);
        if (knonce == NULL) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        size_t kct_size = sqlite3_column_bytes(stmt, 1);
        if (kct_size <= PADDING_BLOCK_SIZE) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        const uint8_t* kct = sqlite3_column_blob(stmt, 1);
        if (kct == NULL) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        guarded_result_t gr = sodium_alloc_slice(kct_size - crypto_secretbox_MACBYTES);
        if (IS_ERROR(gr)) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }
        key = UNWRAP(gr);

        if (crypto_secretbox_open_easy(key.ptr, kct, kct_size, knonce, v->encrypt_key.ptr) != 0) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        size_t unpad_size = 0;
        if (sodium_unpad(&unpad_size, key.ptr, key.cap, PADDING_BLOCK_SIZE) != 0) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }
        key = set_slice_len(key, unpad_size);

        if (prefix.len > key.len || memcmp(prefix.ptr, key.ptr, prefix.len) != 0) {
            rc = sqlite3_step(stmt);
            continue;
        }

        // Decrypt Value
        if (sqlite3_column_bytes(stmt, 2) != crypto_secretbox_NONCEBYTES) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        const uint8_t* vnonce = sqlite3_column_blob(stmt, 2);
        if (vnonce == NULL) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        size_t vct_size = sqlite3_column_bytes(stmt, 3);
        if (vct_size <= PADDING_BLOCK_SIZE) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        const uint8_t* vct = sqlite3_column_blob(stmt, 3);
        if (vct == NULL) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        gr = sodium_alloc_slice(vct_size - crypto_secretbox_MACBYTES);
        if (IS_ERROR(gr)) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }
        value = UNWRAP(gr);

        if (crypto_secretbox_open_easy(value.ptr, vct, vct_size, vnonce, v->encrypt_key.ptr) != 0) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }

        unpad_size = 0;
        if (sodium_unpad(&unpad_size, value.ptr, value.cap, PADDING_BLOCK_SIZE) != 0) {
            result = ERROR(nil_result_t, -1);
            goto cleanup;
        }
        value = set_slice_len(value, unpad_size);

        iter(ctx, key, value);

        rc = sqlite3_step(stmt);
    } 
    
    if (rc == SQLITE_DONE) {
        result = NIL(nil_result_t);
        goto cleanup;
    } else {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

cleanup:
    sodium_free_slice(value);
    sodium_free_slice(key);
    guarded_no_access(v->encrypt_key);
    guarded_no_access(v->signing_key);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

nil_result_t vault_set (vault_t* v, slice key, slice value) {
    nil_result_t result = {0};

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    guarded_t signature = {0};
    guarded_t padded_value = {0};
    guarded_t padded_key = {0};

    slice value_ct = {0};
    slice key_ct = {0};

    nil_result_t nr = guarded_read_only(v->encrypt_key);
    if (IS_ERROR(nr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    nr = guarded_read_only(v->signing_key);
    if (IS_ERROR(nr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }

    // Hash the key
    guarded_result_t gr = sodium_alloc_slice(crypto_generichash_BYTES);
    if (IS_ERROR(gr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    signature = UNWRAP(gr);

    if (crypto_generichash(signature.ptr, signature.cap, key.ptr, key.len, v->signing_key.ptr, v->signing_key.len) != 0) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    signature = set_slice_len(signature, signature.cap);

    // Pad the value
    gr = sodium_alloc_slice((PADDING_BLOCK_SIZE * 2) + value.len);
    if (IS_ERROR(gr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    padded_value = UNWRAP(gr);

    memcpy(padded_value.ptr, value.ptr, value.len);
    padded_value = set_slice_len(padded_value, value.len);

    size_t pad_size = 0;
    if (sodium_pad(&pad_size, padded_value.ptr, padded_value.len, PADDING_BLOCK_SIZE, padded_value.cap) != 0) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    padded_value = set_slice_len(padded_value, pad_size);

    // Encrypt the value
    uint8_t val_nonce[crypto_secretbox_NONCEBYTES] = {0};
    randombytes_buf(val_nonce, sizeof (val_nonce));

    slice_result_t sr = alloc_slice(padded_value.len + crypto_secretbox_MACBYTES);
    if (IS_ERROR(sr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    value_ct = UNWRAP(sr);

    if (crypto_secretbox_easy(value_ct.ptr, padded_value.ptr, padded_value.len, val_nonce, v->encrypt_key.ptr) != 0) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    value_ct = set_slice_len(value_ct, value_ct.cap);

    padded_value = sodium_free_slice(padded_value);

    // Pad the key
    gr = sodium_alloc_slice((PADDING_BLOCK_SIZE * 2) + key.len);
    if (IS_ERROR(gr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    padded_key = UNWRAP(gr);

    memcpy(padded_key.ptr, key.ptr, key.len);
    padded_key = set_slice_len(padded_key, key.len);

    pad_size = 0;
    if (sodium_pad(&pad_size, padded_key.ptr, padded_key.len, PADDING_BLOCK_SIZE, padded_key.cap) != 0) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    padded_key = set_slice_len(padded_key, pad_size);

    // Encrypt the key
    uint8_t key_nonce[crypto_secretbox_NONCEBYTES] = {0};
    randombytes_buf(key_nonce, sizeof (key_nonce));

    sr = alloc_slice(padded_key.len + crypto_secretbox_MACBYTES);
    if (IS_ERROR(sr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    key_ct = UNWRAP(sr);

    if (crypto_secretbox_easy(key_ct.ptr, padded_key.ptr, padded_key.len, key_nonce, v->encrypt_key.ptr) != 0) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    key_ct = set_slice_len(key_ct, key_ct.cap);

    padded_key = sodium_free_slice(padded_key);

    // Store the encrypted key and value
    int rc = sqlite3_open(v->db_file, &db);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_prepare_v2(db, set_value_query, strlen(set_value_query), &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_bind_blob(stmt, 1, signature.ptr, signature.len, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }
    rc = sqlite3_bind_blob(stmt, 2, key_nonce, sizeof (key_nonce), SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }
    rc = sqlite3_bind_blob(stmt, 3, key_ct.ptr, key_ct.len, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }
    rc = sqlite3_bind_blob(stmt, 4, val_nonce, sizeof (val_nonce), SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }
    rc = sqlite3_bind_blob(stmt, 5, value_ct.ptr, value_ct.len, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    result = NIL(nil_result_t);

cleanup:
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    guarded_no_access(v->encrypt_key);
    guarded_no_access(v->signing_key);
    free_slice(value_ct);
    free_slice(key_ct);
    sodium_free_slice(padded_key);
    sodium_free_slice(padded_value);
    sodium_free_slice(signature);
    return result;
}

nil_result_t vault_del (vault_t* v, slice key) {
    nil_result_t result = {0};

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    guarded_t signature = {0};

    nil_result_t nr = guarded_read_only(v->signing_key);
    if (IS_ERROR(nr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }

    guarded_result_t gr = sodium_alloc_slice(crypto_generichash_BYTES);
    if (IS_ERROR(gr)) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    signature = UNWRAP(gr);

    if (crypto_generichash(signature.ptr, signature.cap, key.ptr, key.len, v->signing_key.ptr, v->signing_key.len) != 0) {
        result = ERROR(nil_result_t, -1);
        goto cleanup;
    }
    signature = set_slice_len(signature, signature.cap);

    int rc = sqlite3_open(v->db_file, &db);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_prepare_v2(db, del_value_query, strlen(del_value_query), &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_bind_blob(stmt, 1, signature.ptr, signature.len, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        result = ERROR(nil_result_t, rc);
        goto cleanup;
    }

    result = NIL(nil_result_t);

cleanup:
    sodium_free_slice(signature);
    guarded_no_access(v->signing_key);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

slice_result_t vault_meta_get (vault_t* v, const char* const key) {
    slice_result_t result = {0};

    slice value = {0};

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_open(v->db_file, &db);
    if (rc != SQLITE_OK) {
        result = ERROR(slice_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_prepare_v2(db, get_metadata_query, strlen(get_metadata_query), &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = ERROR(slice_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        result = ERROR(slice_result_t, rc);
        goto cleanup;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        size_t byte_size = sqlite3_column_bytes(stmt, 0);

        slice_result_t sr = alloc_slice(byte_size);
        if (IS_ERROR(sr)) {
            result = ERROR(slice_result_t, -1);
            goto cleanup;
        }
        value = UNWRAP(sr);

        const uint8_t* val_buf = sqlite3_column_blob(stmt, 0);
        if (val_buf == NULL) {
            result = ERROR(slice_result_t, -1);
            goto cleanup;
        }

        memcpy(value.ptr, val_buf, value.cap);
        value = set_slice_len(value, value.cap);

        result = VALUE(slice_result_t, value);
        goto success;
    } else if (rc == SQLITE_DONE) {
        result = NIL(slice_result_t);
        goto cleanup;
    } else {
        result = ERROR(slice_result_t, rc);
        goto cleanup;
    }

cleanup:
    free_slice(value);
success:
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

/*nil_result_t vault_meta_set (vault_t* v, const char* const key, slice value) {
    return NIL(nil_result_t);
}*/

void free_value (guarded_t val) {
    sodium_free(val.ptr);
}

void free_metadata (slice meta) {
    free(meta.ptr);
}

guarded_result_t sodium_alloc_slice (size_t capacity) {
    uint8_t* ptr = sodium_malloc(capacity);
    if (ptr == NULL) {
        return ERROR(guarded_result_t, -1);
    }
    return VALUE(guarded_result_t, slice_ptr(ptr, 0, capacity));
}

guarded_t sodium_free_slice(guarded_t s) {
    sodium_free(s.ptr);
    return (guarded_t){.ptr=NULL, .len=0, .cap=0};
}

slice_result_t alloc_slice (size_t capacity) {
    uint8_t* ptr = malloc(capacity);
    if (ptr == NULL) {
        return ERROR(slice_result_t, -1);
    }
    return VALUE(slice_result_t, slice_ptr(ptr, 0, capacity));
}

slice free_slice(slice s) {
    free(s.ptr);
    return (guarded_t){.ptr=NULL, .len=0, .cap=0};
}

nil_result_t guarded_read_only  (guarded_t g) {
    if (sodium_mprotect_readonly(g.ptr) != 0) {
        return ERROR(nil_result_t, -1);
    }

    return NIL(nil_result_t);
}
nil_result_t guarded_read_write (guarded_t g) {
    if (sodium_mprotect_readwrite(g.ptr) != 0) {
        return ERROR(nil_result_t, -1);
    }

    return NIL(nil_result_t);
}
nil_result_t guarded_no_access  (guarded_t g) {
    if (sodium_mprotect_noaccess(g.ptr) != 0) {
        return ERROR(nil_result_t, -1);
    }

    return NIL(nil_result_t);
}