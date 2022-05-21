#pragma once

#include <stdbool.h>
#include <assert.h>

// Macros for generating sum types for function returns
// The macros will define types, and helper functions for accessing the summed types
// Currently the sums that are available are error+value and error+nil+value

#define NIL_TYPE uint8_t

#define DEFINE_RESULT_SUM_TYPE(value_type, sum_type_name) \
typedef struct sum_type_name { \
    bool is_error; \
    bool is_null; \
    union { \
        int error; \
        value_type value; \
    } sum; \
} sum_type_name;

#define DEFINE_RESULT_ARR_SUM_TYPE(value_type, value_size, sum_type_name) \
typedef struct sum_type_name { \
    bool is_error; \
    bool is_null; \
    union { \
        int error; \
        value_type value[value_size]; \
    } sum; \
} sum_type_name;

#define RETURN_ERROR(sum_type, return_type) \
if sum_type.is_error { \
    return (return_type){.is_error=true, .sum={.error=sum_type.sum.error}}; \
}

#define UNWRAP(sum_type)   (assert(!sum_type.is_error && !sum_type.is_null), sum_type.sum.value)
#define IS_NULL(sum_type)  (sum_type.is_null)
#define IS_ERROR(sum_type) (sum_type.is_error)

#define ERROR(sum_type, error_code) (sum_type){.is_error=true,  .is_null=false, .sum={.error=error_code}}
#define NIL(sum_type)               (sum_type){.is_error=false, .is_null=true}
#define VALUE(sum_type, var_value)      (sum_type){.is_error=false, .is_null=false, .sum={.value=var_value}}