#ifndef PASSMNGR_RESULT_H_
#define PASSMNGR_RESULT_H_

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define OPTIONAL_TYPE(N, V) \
    typedef struct N { \
        bool is_ok; \
        V value; \
    } N;

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

#define OK(R, V)         (R.is_ok = true, R.result.value = (V), (R))
#define ERR(R, E)        (R.is_ok = false, R.result.error = (E), (R))
#define OK_EMPTY(R)      (R.is_ok = true, (R))
#define IS_OK(R)         (R.is_ok)
#define IS_ERR(R)        (!R.is_ok)
#define UNWRAP(R)        (assert(IS_OK(R)), R.result.value)
#define UNWRAP_EMPTY(R)  (assert(IS_OK(R)))
#define UNWRAP_ERR(R)    (assert(IS_ERR(R)), R.result.error)

#define SET_ERR(R, E)    (R.is_ok = false, R.result.error = (E))
#define SET_OK(R, V)     (R.is_ok = true, R.result.value = (V))
#define SET_OK_EMPTY(R)  (R.is_ok = true)

#endif
