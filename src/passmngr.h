#ifndef __PASSMNGR
#define __PASSMNGR

#include <stdint.h>
#include <result.h>

typedef enum entry_op {
    SET = 0x00,
    GET = 0x01
} entry_op_t;

typedef enum passmngr_err {
    FILE_READ_ERROR,
    FILE_WRITE_ERROR,
    FILE_OPEN_ERROR,
    CRYPTO_ERROR,
    FILE_CONTENT_ERROR
} pm_error_t;

// The type used to store string size will impose limits on the possible length of a string
typedef uint16_t string_size_t;

// 'length' will always be used to define the number of bytes something occupies, for example
// string 'length's will indicate the number of bytes in the string not number of codepoints

// 'count' will always be the number of items in a set, for example string 'count's will
// indicate the number of codepoints not the number of bytes

typedef struct entry {
    entry_op_t op;
    string_size_t key_length;
    string_size_t value_length;
    void* key_buffer;
    void* value_buffer;
} entry_t;

typedef struct string_slice {
    string_size_t length;
    void* value;
} string_slice_t;

typedef struct passmngr {
    FILE * stream;

} passmngr_t;

RESULT_TYPE(str_result_t, string_slice_t, pm_error_t);
RESULT_EMPTY_TYPE(error_result_t, pm_error_t);

err_result_t pm_open        (passmngr_t* pm, const char* filename);
err_result_t pm_unlock      (passmngr_t* pm, const void* master_password, size_t password_length);
err_result_t pm_lock        (passmngr_t* pm);
err_result_t pm_close       (passmngr_t* pm);

str_result_t pm_get         (passmngr_t* pm, string_slice_t key);
err_result_t pm_set         (passmngr_t* pm, string_slice_t key, string_slice_t value);
err_result_t pm_batched_set (passmngr_t* pm, size_t count, string_slice_t* keys, string_slice_t* values);

#endif
