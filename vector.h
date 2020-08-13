#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "result.h"

#define vector_type(T, N) \
    result_type(T, N); \
    typedef struct {T* buffer; size_t length; size_t capacity; } vector_##N##_t; \
    \
    bool vector_##N##_expand(vector_##N##_t* v) { \
        size_t new_cap = v->capacity > 0 ? v->capacity * 2 : 1; \
        T* tmp = realloc(v->buffer, sizeof (T) * new_cap); \
        if (tmp == NULL) { \
            return false; \
        } else { \
            v->capacity = new_cap; \
            v->buffer = tmp; \
            return true; \
        } \
    } \
    bool vector_##N##_shrink(vector_##N##_t* v) { \
        size_t new_cap = v->capacity / 2; \
        if (!new_cap) return true; \
        if (v->length >= new_cap) { \
            return true; \
        } \
        T* tmp = realloc(v->buffer, sizeof (T) * new_cap); \
        if (tmp == NULL) { \
            return false; \
        } else { \
            v->capacity = new_cap; \
            v->buffer = tmp; \
            return true; \
        } \
    } \
    bool vector_##N##_push(vector_##N##_t* v, T val) { \
        if (v->length < v->capacity) { \
            v->buffer[v->length] = val; \
            v->length++; \
            return true; \
        } else if (vector_##N##_expand(v)) { \
            return vector_##N##_push(v, val); \
        } else { \
            return false; \
        } \
    } \
    result_##N##_t vector_##N##_pop(vector_##N##_t* v) { \
        if (v->length < 0) { \
            return result_##N##_error(); \
        } \
        T val = v->buffer[v->length - 1]; \
        v->length--; \
        return result_##N##_ok(val); \
    } \
    result_##N##_t vector_##N##_get(vector_##N##_t* v, size_t i) { \
        if (v->length <= i) { \
            return result_##N##_error(); \
        } \
        return result_##N##_ok(v->buffer[i];); \
    } \
    bool vector_##N##_set(vector_##N##_t* v, size_t i, T val) { \
        if (i > v->length) { \
            return false; \
        } else if (i == v->length) { \
            return vector_##N##_push(v, val); \
        } else { \
            v->buffer[i] = val; \
            return true; \
        } \
    } \


#endif