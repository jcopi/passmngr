#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "result.h"
#include <string.h>

typedef enum vector_error {
    INDEX_OUT_OF_RANGE,
    ALLOCATION_FAILURE
} vector_error_t;

#define vector_type(T, N) \
    result_type(T, vector_error_t, v_##N); \
    typedef struct {T* buffer; size_t length; size_t capacity; } vector_##N##_t; \
    result_type(vector_##N##_t, vector_error_t, ##N); \
    \
    result_v_##N##_t _vector_##N##_expand(vector_##N##_t* v) { \
        size_t new_cap = v->capacity > 0 ? v->capacity * 2 : 1; \
        T* tmp = realloc(v->buffer, sizeof (T) * new_cap); \
        if (tmp == NULL) { \
            return result_v_##N##_err(ALLOCATION_FAILURE); \
        } else { \
            v->capacity = new_cap; \
            v->buffer = tmp; \
            return result_v_##N##_ok(v) \
        } \
    } \
    result_v_##N##_t _vector_##N##_shrink(vector_##N##_t v) { \
        size_t new_cap = v->capacity / 2; \
        if (!new_cap || v->length >= new_cap) { \
            return result_v_##N##_ok(v); \
        } \
        T* tmp = realloc(v->buffer, sizeof (T) * new_cap); \
        if (tmp == NULL) { \
            return result_v_##N##_err(ALLOCATION_FAILURE); \
        } else { \
            v->capacity = new_cap; \
            v->buffer = tmp; \
            return result_v_##N##_ok(v); \
        } \
    } \
    result_v_##N##_t vector_##N##_push(vector_##N##_t v, T val) { \
        if (v.length < v.capacity) { \
            v.buffer[v.length] = val; \
            v.length++; \
            return result_v_##N##_ok(v); \
        } else { \
            result_v_##N##_t n_v = vector_##N##_expand(v);\
            if (n_v.is_ok()) { \
                return vector_##N##_push(n_v.expect(), val); \
            } \
            return n_v; \
        } \
    } \
    result_##N##_t vector_##N##_pop(vector_##N##_t v) { \
        if (v.length <= 0) { \
            return result_##N##_err(INDEX_OUT_OF_RANGE); \
        } \
        T val = v.buffer[v.length - 1]; \
        v.length--; \
        return result_##N##_ok(val); \
    } \
    result_##N##_t vector_##N##_get(vector_##N##_t v, size_t i) { \
        if (v.length <= i) { \
            return result_##N##_err(INDEX_OUT_OF_RANGE); \
        } \
        return result_##N##_ok(v.buffer[i]); \
    } \
    result_v_##N##_t vector_##N##_set(vector_##N##_t v, size_t i, T val) { \
        if (i >= v.length) { \
            return result_v_##N##_err(INDEX_OUT_OF_RANGE); \
        } else { \
            v.buffer[i] = val; \
            return result_v_##N##_ok(v); \
        } \
    } \


#endif