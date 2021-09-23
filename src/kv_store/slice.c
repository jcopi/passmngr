#include <slice.h>

inline slice_t s_create (const void* ptr, size_t cap, size_t len) {
    return (slice_t){.ptr=(uint8_t*)ptr, .cap=cap, .len=len};
}
inline slice_t s_slice_l (slice_t s, size_t start, size_t len) {
    assert(s.len >= start + len);
    return (slice_t){.ptr=s.ptr+start, .cap=s.cap-start, .len=len};
}
inline slice_t s_slice_i (slice_t s, size_t start, size_t end) {
    assert(end >= start && s.len >= end);
    return (slice_t){.ptr=s.ptr+start, .cap=s.cap-start, .len=end-start};
}
inline slice_t s_append (slice_t dest, slice_t src, size_t count) {
    assert(count <= src.len && count <= dest.cap-dest.len);
    memmove(dest.ptr+dest.len, src.ptr, count);
    return (slice_t){.ptr=dest.ptr, .cap=dest.cap, .len=dest.len+count};
}
inline slice_t s_consume_start (slice_t s, size_t count) {
    assert(count <= s.len);
    return (slice_t){.ptr=s.ptr+count, .cap=s.cap-count, .len=s.len-count};
}
inline slice_t s_consume_end (slice_t s, size_t count) {
    assert(count <= s.len);
    return (slice_t){.ptr=s.ptr, .cap=s.cap, .len=s.len-count};
}
inline slice_t s_produce_end (slice_t s, size_t count) {
    assert(count <= s.cap-s.len);
    return (slice_t){.ptr=s.ptr, .cap=s.cap, .len=s.len+count};
}