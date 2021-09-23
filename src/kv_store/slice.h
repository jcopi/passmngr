#pragma once

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

typedef struct slice {
    uint8_t* const ptr;
    const size_t len;
    const size_t cap;
} slice_t;

slice_t s_create  (const void* ptr, size_t cap, size_t len);
slice_t s_slice_l (slice_t s, size_t start, size_t len);
slice_t s_slice_i (slice_t s, size_t start, size_t end);
slice_t s_append  (slice_t dest, slice_t src, size_t count);

slice_t s_consume_start (slice_t s, size_t count);
slice_t s_consume_end   (slice_t s, size_t count);
slice_t s_produce_end   (slice_t s, size_t count);
