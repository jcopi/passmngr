#ifndef PASSMNGR_ARCHIVE_H_
#define PASSMNGR_ARCHIVE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <hashmap.h>
#include <result.h>
#include <common.h>

#define AR_FIXED_CHUNK_SIZE (512)

typedef struct archive_record {
    uint64_t size;
    byte_t*  bytes;
    uint64_t prev;
    uint64_t next;
} archive_record_t;

typedef struct archive {
    bool opened;
    bool borrowed;

    file_mode_t mode;
    
    FILE* file;
    byte_t buffer[AR_FIXED_CHUNK_SIZE];

    map_t index;
} archive_t;

typedef struct archive_item {
    archive_t* parent;
    uint64_t root;
    uint64_t start;
    uint64_t current;
} archive_item_t;

typedef enum archive_error {
    AR_OUT_OF_MEMORY = COMMON_ARCHIVE_ERROR_START,
    AR_FILE_OPEN_FAILED,
    AR_FILE_READ_FAILED,
    AR_FILE_WRITE_FAILED,
    AR_FILE_SEEK_FAILED,
    AR_INVALID_OPERATION,
    AR_ITEM_ALREADY_OPEN,
    AR_INVALID_FORMAT,
    AR_INVALID_MODE
} archive_error_t;

RESULT_TYPE(ar_size_result_t, size_t, archive_error_t)
RESULT_TYPE(ar_item_result_t, archive_item_t, archive_error_t);
RESULT_TYPE(ar_record_result_t, archive_record_t, archive_error_t);
RESULT_EMPTY_TYPE(ar_empty_result_t, archive_error_t);

ar_empty_result_t archive_open  (archive_t* ar, const char* file_name, const file_mode_t mode);
ar_empty_result_t archive_close (archive_t* ar);

ar_item_result_t  archive_item_open   (archive_t* ar, const byte_t* const name, const size_t name_size);
ar_empty_result_t archive_item_close  (archive_item_t* item);
ar_size_result_t  archive_item_read   (archive_item_t* item, const byte_t* buffer, const size_t buffer_size);
ar_size_result_t  archive_item_write  (archive_item_t* item, const byte_t* const buffer, const size_t buffer_size);
ar_empty_result_t archive_item_delete (archive_item_t* item);
ar_empty_result_t archive_item_splice (archive_item_t* item, const uint64_t start, const uint64_t size);

bool archive_has_item (archive_t* ar, const byte_t* const name, const size_t name_size);

#endif
