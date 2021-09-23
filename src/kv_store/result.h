#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <slice.h>
#include <kv_types.h>

typedef struct optional_item {
    bool is_available;
    kv_item_t item;
} optional_item_t;

typedef struct result_item {
    bool is_error;
    union {
        kv_error_t error;
        kv_item_t item;
    } maybe;
} result_item_t;

typedef struct result_opt_item {
    bool is_error;
    union {
        kv_error_t error;
        optional_item_t opt_item;
    } maybe;
} result_opt_item_t;

typedef struct result_uint64 {
    bool is_error;
    union {
        kv_error_t error;
        uint64_t uint64;
    } maybe;
} result_uint64_t;

typedef struct result_slice {
    bool is_error;
    union {
        kv_error_t error;
        slice_t slice;
    } maybe;
} result_slice_t;

kv_item_t optional_item_unwrap (optional_item_t optional);

kv_item_t result_item_unwrap (result_item_t result);

optional_item_t result_opt_item_unwrap (result_opt_item_t result);

uint64_t result_uint64_unwrap (result_uint64_t result);

slice_t result_slice_unwrap (result_slice_t result);

kv_error_t result_item_unwrap_err (result_item_t result);

kv_error_t result_opt_item_unwrap_err (result_opt_item_t result);

kv_error_t result_uint64_unwrap_err (result_uint64_t result);

kv_error_t result_slice_unwrap_err (result_slice_t result);

result_uint64_t result_uint64_ok (uint64_t value);
result_uint64_t result_uint64_error (kv_error_t error);

result_item_t result_item_ok (kv_item_t value);
result_item_t result_item_error (kv_error_t error);

optional_item_t optional_item_ok (kv_item_t value);
optional_item_t optional_item_empty (void);

result_opt_item_t result_opt_item_ok (optional_item_t value);
result_opt_item_t result_opt_item_error (kv_error_t error);

result_slice_t result_slice_ok (slice_t value);
result_slice_t result_slice_error (kv_error_t error);
