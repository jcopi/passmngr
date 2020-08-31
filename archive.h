#ifndef __PASSMNGR_ARCHIVE_H
#define __PASSMNGR_ARCHIVE_H

#include "hashmap/hashmap.h"
#include "result.h"
#include <stdlib.h>
#include <stdbool.h>

struct archive_entity {
    size_t start;
    size_t header_start;
    uint64_t length;
};

enum archive_mode {
    READ,
    WRITE,
};

enum archive_state {
    IDLE,
    ARCHIVE_OPEN,
    ITEM_OPEN,
};

typedef enum archive_error {
    READ_FAILED,
    WRITE_FAILED,
    SEEK_FAILED,
    ALLOCATION_FAILED,
} archive_error_t;

typedef struct archive {
    map_t entities;
    FILE file_stream;
    enum archive_mode mode;
    enum archive_state state;
} archive_t;

result_t_init(archive_t, archive_error_t);
result_t_init(nil_t, archive_error_t);

result_t(archive_t, archive_error_t) archive_create(const char* name);
result_t(archive_t, archive_error_t) archive_open  (const char* name);

result_t(nil_t, archive_error_t) archive_close (archive_t* a);



// int archive_init   (struct archive* ar);
//int archive_create (struct archive* ar, const char* name);
//int archive_open   (struct archive* ar, const char* name);
// int archive_close  (struct archive* ar);

int archive_create_item (struct archive* ar, const void* name, size_t name_bytes);
int archive_write_item  (struct archive* ar, void* buffer, size_t buffer_bytes);
int archive_close_item  (struct archive* ar);

int archive_open_item  (struct archive* ar, const void* name, size_t name_bytes);
int archive_read_item  (struct archive* ar, void* buffer, size_t buffer_bytes);
int archive_close_item (struct archive* ar);

static int archive_populate_items (struct archive* ar);

#endif