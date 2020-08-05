#include "archive.h"
#include <stdlib.h>

#define ARCHIVE_ITEM_NAME_MAXBYTES  (128)

int archive_init (struct archive* ar)
{
    if (hm_init(&ar->entities) != 0) {
        return -1;
    }

    ar->entities.deallocator = free;
    ar->file_stream = NULL;
    return 0;
}
int archive_create (struct archive* ar, const char* name)
{
    ar->mode = WRITE;
    ar->file_stream = fopen(name, "wb");
    if (ar->file_stream == NULL) {
        return -1;
    }

    return 0;
}

int archive_open (struct archive* ar, const char* name)
{
    ar->mode = READ;
    ar->file_stream = fopen(name, "rb");
    if (ar->file_stream == NULL) {
        return -1;
    }

    return archive_populate_items(ar);
}

static int archive_populate_items (struct archive* ar)
{
    size_t rlen;
    uint16_t name_bytes;
    uint64_t item_bytes;
    int eof;

    do {
        rlen = fread(&name_bytes, sizeof (name_bytes), 1, ar->file_stream);
        eof = feof(ar->file_stream);

        if (rlen != sizeof (name_bytes) || eof || name_bytes > ARCHIVE_ITEM_NAME_MAXBYTES) {
            return -1;
        }

        char* item_name = calloc(name_bytes + 1, sizeof (char));
        if (item_name == NULL) {
            return -1;
        }

        rlen = fread(item_name, sizeof (char), name_bytes, ar->file_stream);
        eof = feof(ar->file_stream);
        if (rlen != name_bytes || eof) {
            free(item_name);
            return -1;
        }

        rlen = fread(&item_bytes, sizeof (item_bytes), 1, ar->file_stream);
        eof = feof(ar->file_stream);
        if (rlen != sizeof (item_bytes) || (eof && item_bytes != 0)) {
            free(item_name);
            return -1;
        }

        struct archive_entity* entity = calloc(sizeof (struct archive_entity), 1);
        if (entity == NULL) {
            free(item_name);
            return -1;
        }
        entity->start = ftell(ar->file_stream);
        entity->length = item_bytes;

        if (hm_set(&ar->entities, item_name, entity) != 0) {
            free(item_name);
            free(entity);
            return -1;
        }

        if (fseek(ar->file_stream, item_bytes, SEEK_CUR) != 0) {
            return -1;
        }

        eof = feof(ar->file_stream);
    } while (!eof);

    return 0;
}

int archive_close (struct archive* ar);

int archive_create_item (struct archive* ar, const char* name);
int archive_write_item  (struct archive* ar, void* buffer, size_t buffer_bytes);
int archive_close_item  (struct archive* ar);

int archive_open_item  (struct archive* ar, const char* name);
int archive_read_item  (struct archive* ar, void* buffer, size_t buffer_bytes);
int archive_close_item (struct archive* ar);