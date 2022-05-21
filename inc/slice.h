#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <sodium.h>

// A slice is a memory region with a length and a capacity.
// items can be written in up to the capacity and read up to the length.
typedef struct slice {
    void* ptr;
    size_t len;
    size_t cap;
} slice;

inline slice slice_ptr (void* const ptr, size_t length, size_t capacity) {
    return (slice){.ptr = ptr, .len = length, .cap = capacity};
}
inline slice clear_slice (slice s) {
    sodium_memzero(s.ptr, s.cap);
    return (slice){.ptr=s.ptr, .len=0, .cap=s.cap};
}
inline slice set_slice_len (slice s, size_t len) {
    assert(len <= s.cap);
    return (slice){.ptr=s.ptr, .len=len, .cap=s.cap};
}


inline slice sub_slice (slice s, size_t start, size_t end) {
    assert(start <= end);
    assert(end <= s.len);

    return (slice){.ptr=((uint8_t*)s.ptr) + start, .len=end-start, .cap=end-start};
}