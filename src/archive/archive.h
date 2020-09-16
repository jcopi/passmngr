#ifndef PASSMNGR_ARCHIVE_H_
#define PASSMNGR_ARCHIVE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define INDEX_SIZE_TYPE uint64_t
#define NAME_SIZE_TYPE  uint16_t
#define ITEM_SIZE_TYPE  uint64_t
#define ITEM_START_TYPE uint64_t

typedef uint8_t byte_t;
static_assert(sizeof (byte_t) == 1, "Invalid byte size");

typedef enum archive_mode {
    READ,
    WRITE,
} archive_mode_t;

typedef enum archive_error {
    FILE_READ_FAILED,
    FILE_WRITE_FAILED,
    FILE_OPEN_FAILED,
    FILE_SEEK_FAILED,

    RUNTIME_ITEM_NOT_FOUND,
    RUNTIME_END_OF_ITEM,
    RUNTIME_ITEM_PREV_OPENED,

    FATAL_INDEX_MALFORMED,
    FATAL_OUT_OF_MEMORY,
    FATAL_UNEXPECTED,
} archive_error_t;

typedef struct archive_item_info {
    ITEM_START_TYPE start;
    ITEM_SIZE_TYPE  bytes;
    NAME_SIZE_TYPE  name_bytes;
    byte_t          name[];
} archive_item_info_t;

typedef struct archive {
    FILE* file;
    archive_mode_t mode;

    bool opened;
    bool locked;
    bool indexed;

    INDEX_SIZE_TYPE index_bytes;
    size_t items_bytes;

    size_t item_count;
    archive_item_info_t** items;
} archive_t;

typedef struct archive_item {
    archive_t* parent;
    archive_item_info_t* info;

    uint64_t current;
} archive_item_t;

typedef struct archive_result {
    bool is_ok;
    union {
        archive_t value;
        archive_error_t error;
    } result;
} archive_result_t;

typedef struct item_result {
    bool is_ok;
    union {
        archive_item_t value;
        archive_error_t error;
    } result;
} item_result_t;

typedef struct info_ref_result {
    bool is_ok;
    union {
        archive_item_info_t* value;
        archive_error_t error;
    } result;
} info_ref_result_t;

typedef struct size_result {
    bool is_ok;
    union {
        size_t value;
        archive_error_t error;
    } result;
} size_result_t;

typedef struct empty_result {
    bool is_ok;
    union {
        archive_error_t error;
    } result;
} empty_result_t;

#define OK(R, V)      (R.is_ok = true, R.result.value = V, R)
#define ERR(R, E)     (R.is_ok = false, R.result.error = E, R)
#define OK_EMPTY(R)   (R.is_ok = true, R)
#define IS_OK(R)      (R.is_ok)
#define IS_ERR(R)     (!R.is_ok)
#define UNWRAP(R)     (assert(IS_OK(R)), R.result.value)
#define UNWRAP_ERR(R) (assert(IS_ERR(R)), R.result.error)

archive_result_t archive_open  (const char* file_name, archive_mode_t mode);
empty_result_t   archive_close (archive_t* ar);

item_result_t  archive_item_open  (archive_t* ar, const byte_t* name, NAME_SIZE_TYPE name_bytes);
size_result_t  archive_item_read  (archive_item_t* item, byte_t* buffer, size_t buffer_bytes);
size_result_t  archive_item_write (archive_item_t* item, const byte_t* buffer, size_t buffer_bytes);
empty_result_t archive_item_close (archive_item_t* item);

#endif