#ifndef PASSMNGR_ARCHIVE_H_
#define PASSMNGR_ARCHIVE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <hashmap.h>
#include <result.h>
#include <common.h>

typedef struct archive_record {
    uint64_t size;
    byte_t*  bytes;
    uint64_t prev;
    uint64_t next;
} archive_record_t;

typedef struct archive {
    FILE* file;

} archive_t;

typedef struct archive_item {

} archive_item_t;

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