#ifndef PASSMNGR_RESULT_H_
#define PASSMNGR_RESULT_H_

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define RESULT_TYPE(N, V, E) \
    typedef struct N { \
        bool is_ok; \
        union { \
            V value;\
            E error; \
        } result; \
    } N;

#define RESULT_EMPTY_TYPE(N, E) \
    typedef struct N { \
        bool is_ok; \
        union { \
            E error; \
        } result; \
    } N; 

#define OK(R, V)      (R.is_ok = true, R.result.value = V, R)
#define ERR(R, E)     (R.is_ok = false, R.result.error = E, R)
#define OK_EMPTY(R)   (R.is_ok = true, R)
#define IS_OK(R)      (R.is_ok)
#define IS_ERR(R)     (!R.is_ok)
#define UNWRAP(R)     (assert(IS_OK(R)), R.result.value)
#define UNWRAP_ERR(R) (assert(IS_ERR(R)), R.result.error)

#endif