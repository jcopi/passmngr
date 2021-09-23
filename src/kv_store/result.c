#include <result.h>

kv_item_t optional_item_unwrap (optional_item_t optional) {
    assert(optional.is_available);
    return optional.item;
}

kv_item_t result_item_unwrap (result_item_t result) {
    assert(result.is_error == false);
    return result.maybe.item;
}

optional_item_t result_opt_item_unwrap (result_opt_item_t result) {
    assert(result.is_error == false);
    return result.maybe.opt_item;
}

uint64_t result_uint64_unwrap (result_uint64_t result) {
    assert(result.is_error == false);
    return result.maybe.uint64;
}

slice_t result_slice_unwrap (result_slice_t result) {
    assert(result.is_error == false);
    return result.maybe.slice;
}

kv_error_t result_item_unwrap_err (result_item_t result) {
    assert(result.is_error);
    return result.maybe.error;
}

kv_error_t result_opt_item_unwrap_err (result_opt_item_t result) {
    assert(result.is_error);
    return result.maybe.error;
}

kv_error_t result_uint64_unwrap_err (result_uint64_t result) {
    assert(result.is_error);
    return result.maybe.error;
}

kv_error_t result_slice_unwrap_err (result_slice_t result) {
    assert(result.is_error);
    return result.maybe.error;
}

result_uint64_t result_uint64_ok (uint64_t value) {
    return (result_uint64_t){.is_error=false, .maybe={.uint64=value}};
}
result_uint64_t result_uint64_error (kv_error_t error) {
    return (result_uint64_t){.is_error=false, .maybe={.error=error}};
}

result_item_t result_item_ok (kv_item_t value) {
    return (result_item_t){.is_error=false, .maybe={.item=value}};
}
result_item_t result_item_error (kv_error_t error) {
    return (result_item_t){.is_error=false, .maybe={.error=error}};
}

optional_item_t optional_item_ok (kv_item_t value) {
    return (optional_item_t){.is_available=true, .item=value};
}
optional_item_t optional_item_empty (void) {
    return (optional_item_t){.is_available=false};
}

result_opt_item_t result_opt_item_ok (optional_item_t value) {
    return (result_opt_item_t){.is_error=false, .maybe={.opt_item=value}};
}
result_opt_item_t result_opt_item_error (kv_error_t error) {
    return (result_opt_item_t){.is_error=false, .maybe={.error=error}};
}

result_slice_t result_slice_ok (slice_t value) {
    return (result_slice_t){.is_error=false, .maybe={.slice=value}};
}
result_slice_t result_slice_error (kv_error_t error) {
    return (result_slice_t){.is_error=false, .maybe={.error=error}};
}
