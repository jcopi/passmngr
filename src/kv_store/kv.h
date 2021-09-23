#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <sodium.h>

#include <slice.h>
#include <result.h>

#include <kv_constants.h>
#include <kv_types.h>

kv_error_t kv_init (kv_t* kv, slice_t read_filename, slice_t write_filename);

slice_t kv_key_buffer (kv_t* kv);

kv_error_t kv_unlock (kv_t* kv);
kv_error_t kv_lock (kv_t* kv);

kv_error_t kv_create (kv_t* kv);

result_uint64_t kv_reserve_get (kv_t* kv);
kv_error_t kv_release_get (kv_t* kv, uint64_t reservation);

result_opt_item_t kv_plain_get (kv_t* kv, uint64_t reservation, slice_t key, size_t n);
result_opt_item_t kv_get (kv_t* kv, uint64_t reservation, slice_t key, size_t n);
// result_opt_item_t kv_get_sorted (kv_t* kv, uint64_t reservation, slice_t key, size_t n, int cmp (slice_t a, slice_t b), slice_t after);

kv_error_t kv_plain_set (kv_t* kv, slice_t key, slice_t value);
kv_error_t kv_set (kv_t* kv, slice_t key, slice_t value);

kv_error_t kv_plain_delete (kv_t* kv, slice_t key);
kv_error_t kv_delete (kv_t* kv, slice_t key);

kv_error_t kv_rekey (kv_t* kv, slice_t new_key);

result_slice_t kv_util_hash_password (slice_t key_dest, slice_t salt, slice_t password);
slice_t kv_util_fill_rand (slice_t dest);
uint32_t kv_util_rand_choice (uint32_t min, uint32_t max);