#ifndef PASSMNGR_ARCHIVE_H_
#define PASSMNGR_ARCHIVE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <result.h>
#include <common.h>

#define INDEX_SIZE_TYPE uint64_t
#define ITEM_SIZE_TYPE  uint64_t
#define ITEM_START_TYPE uint64_t

typedef uint8_t byte_t;
static_assert(sizeof (byte_t) == 1, "Invalid byte size");

typedef enum archive_mode {
    READ,
    WRITE,
} archive_mode_t;

typedef enum archive_error {
    FILE_READ_FAILED = COMMON_ARCHIVE_ERROR_START,
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
    COMMON_ITEM_NAME_TYPE  name_bytes;
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


RESULT_TYPE(archive_result_t, archive_t, archive_error_t)
RESULT_TYPE(archive_item_result_t, archive_item_t, archive_error_t)
RESULT_TYPE(archive_info_ref_result_t, archive_item_info_t*, archive_error_t)
RESULT_TYPE(archive_size_result_t, size_t, archive_error_t)
RESULT_EMPTY_TYPE(archive_empty_result_t, archive_error_t)

archive_result_t         archive_open  (const char* file_name, archive_mode_t mode);
archive_empty_result_t   archive_close (archive_t* ar);

archive_item_result_t  archive_item_open  (archive_t* ar, const byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes);
archive_size_result_t  archive_item_read  (archive_item_t* item, byte_t* buffer, size_t buffer_bytes);
archive_size_result_t  archive_item_write (archive_item_t* item, const byte_t* buffer, size_t buffer_bytes);
archive_empty_result_t archive_item_close (archive_item_t* item);

bool                   archive_has_item   (archive_t* ar, const byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes);

#endif
