#ifndef __RESULT_H__
#define __RESULT_H__

#include <stdlib.h>

#define result_type(T, N) \
    typedef struct {bool error; T value} result_##N##_t; \
    result_##N##_t result_##N##_ok(T val) { \
        return {false, val}; \
    } \
    result_##N##_t result_##N##_error() { \
        return {true, (T)0}; \
    } \
    T result_##N##_expect(result_##N##_t r) { \
        if (r.error) exit(EXIT_FAILURE); \
        return r.value; \
    }

#endif