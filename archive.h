#ifndef __PASSMNGR_ARCHIVE_H
#define __PASSMNGR_ARCHIVE_H

#include "hashmap/hashmap.h"

struct archive_entity {
    size_t start;
    uint64_t length;
};

enum archive_mode {
    READ,
    WRITE,
};

struct archive {
    map_t entities;
    FILE* file_stream;
    enum archive_mode mode;
};

int archive_init   (struct archive* ar);
int archive_create (struct archive* ar, const char* name);
int archive_open   (struct archive* ar, const char* name);
int archive_close  (struct archive* ar);

int archive_create_item (struct archive* ar, const char* name);
int archive_write_item  (struct archive* ar, void* buffer, size_t buffer_bytes);
int archive_close_item  (struct archive* ar);

int archive_open_item  (struct archive* ar, const char* name);
int archive_read_item  (struct archive* ar, void* buffer, size_t buffer_bytes);
int archive_close_item (struct archive* ar);

static int archive_populate_items (struct archive* ar);

#endif