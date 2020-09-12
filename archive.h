#ifndef _ARCHIVE_H_
#define _ARCHIVE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/********************************************
 * 
 * 
 * 
 * 
 * 
 * 
 * ******************************************/

// Constants/enums
typedef enum archive_error {
    E_FILE_OPEN_ERROR,
    E_FILE_READ_ERROR,
    E_FILE_WRITE_ERROR,
    E_FILE_SEEK_ERROR,

    F_INDEX_PARSE_ERROR,
    F_INDEX_INCORRECT_ERROR,

    R_ITEM_NOT_FOUND,
    R_END_OF_ITEM,
    R_ITEM_PREV_OPENED,
    R_ALLOCATION_ERROR,
    
    U_INCORRECT_OP_MODE,
    U_ITEM_STILL_OPEN,
    U_ARCHIVE_NOT_OPEN,
    U_ITEM_NOT_OPEN,
} archive_error_t;

typedef enum archive_mode {
    READ,
    WRITE,
} archive_mode_t;

// archive types
typedef struct archive {
    FILE* file;
    archive_mode_t mode;

    bool open;
    bool borrowed;

    uint8_t* names_buffer;
    size_t   names_size;

    archive_item_info_t* items;
    size_t item_count;

    uint64_t index_size;
    uint64_t item_content_size;
} archive_t;

typedef struct archive_item_info {
    uint8_t* name;
    uint16_t name_size;

    uint64_t start;
    uint64_t size;
} archive_item_info_t;

typedef struct archive_item {
    archive_t* parent;
    archive_item_info_t* self;

    uint64_t current;
} archive_item_t;

// result types (return values)
#define IS_OK(R)  (R.ok)
#define IS_ERR(R) (!R.ok)
#define OK(R,V)   (R.ok = true, R.result.value = V, R)
#define ERR(R,E)  (R.ok = false, R.result.error = E, R)
#define OK_EMPTY(R) (R.ok = true, R)

typedef struct archive_result {
    bool ok;
    union {
        archive_t value;
        archive_error_t error;
    } result;
} archive_result_t;

typedef struct item_result {
    bool ok;
    union {
        archive_item_t value;
        archive_error_t error;
    } result;
} item_result_t;

typedef struct info_result {
    bool ok;
    union {
        archive_item_info_t value;
        archive_error_t error;
    } result;
} info_result_t;

typedef struct size_result {
    bool ok;
    union {
        size_t value;
        archive_error_t error;
    } result;
} size_result_t;

typedef struct empty_result {
    bool ok;
    union {
        archive_error_t error;
    } result;
} empty_result_t;

// api
archive_result_t archive_open        (const char* file_name, archive_mode_t mode);
empty_result_t   archive_close       (archive_t* ar);

item_result_t    archive_item_open   (archive_t* ar, const uint8_t* name, uint16_t name_size);
size_result_t    archive_item_read   (archive_item_t* item, uint8_t* buffer, size_t buffer_size);
size_result_t    archive_item_write  (archive_item_t* item, const uint8_t* buffer, size_t buffer_size);
empty_result_t   archive_item_close  (archive_item_t* item);

info_result_t    archive_read_info   (archive_t* ar);
empty_result_t   archive_write_info  (archive_t* ar, archive_item_info_t info);

empty_result_t   archive_parse_index (archive_t* ar);
empty_result_t   archive_write_index (archive_t* ar);

#endif
