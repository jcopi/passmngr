#pragma once

#include <slice.h>
#include <sum_types.h>
#include <sqlite3.h>

typedef slice key_t;
typedef slice guarded_t;

typedef struct vault {
    key_t encrypt_key;
    key_t signing_key;

    const char* db_file;
} vault_t;

typedef struct item {
    slice key;
    slice value;
    int64_t time;

    uint8_t* ptr;
} item_t;

DEFINE_RESULT_SUM_TYPE(vault_t, vault_result_t)
DEFINE_RESULT_SUM_TYPE(slice, slice_result_t)
DEFINE_RESULT_SUM_TYPE(guarded_t, guarded_result_t)
DEFINE_RESULT_SUM_TYPE(NIL_TYPE, nil_result_t)

nil_result_t     init_vault ();

vault_result_t   new_vault  (const char* const db_file, slice password);
guarded_result_t vault_get  (vault_t* v, slice key);
nil_result_t     vault_iter (vault_t* v, slice prefix, void (*iter)(void* ctx, guarded_t key, guarded_t value), void* ctx);
nil_result_t     vault_set  (vault_t* v, slice key, slice value);
nil_result_t     vault_del  (vault_t* v, slice key);

slice_result_t vault_meta_get (vault_t* v, const char* const key);
nil_result_t   vault_meta_set (vault_t* v, const char* const key, slice value);

void destroy_vault (vault_t* v);
void free_value    (guarded_t val);
void free_metadata (slice meta);

guarded_result_t sodium_alloc_slice (size_t capacity);
guarded_t        sodium_free_slice  (guarded_t s);

nil_result_t guarded_read_only  (guarded_t g);
nil_result_t guarded_read_write (guarded_t g);
nil_result_t guarded_no_access  (guarded_t g);