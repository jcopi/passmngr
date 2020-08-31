#include "archive.h"
#include <stdlib.h>

#define ARCHIVE_ITEM_NAME_MAXBYTES  (128)

result_t(archive_t, archive_error_t) archive_create(const char* name)
{
    
}

int archive_init (struct archive* ar)
{
    if (hm_init(&ar->entities) != 0) {
        return -1;
    }

    ar->entities.deallocator = free;
    ar->file_stream = NULL;
    return 0;
}
/*int archive_create (struct archive* ar, const char* name)
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
}*/

static int archive_populate_items (struct archive* ar)
{
    size_t rlen;
    uint16_t name_bytes;
    uint64_t item_bytes;
    size_t hstart;
    int eof;

    do {
        hstart = ftell(ar->file_stream);
        rlen = fread(&name_bytes, sizeof (name_bytes), 1, ar->file_stream);
        eof = feof(ar->file_stream);

        if (rlen != sizeof (name_bytes) || eof || name_bytes > ARCHIVE_ITEM_NAME_MAXBYTES) {
            return -1;
        }

        char* item_name = calloc(name_bytes, sizeof (char));
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
        entity->header_start = hstart;
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

int archive_close (struct archive* ar)
{
    fclose(ar->file_stream);
    ar->file_stream = NULL;
    hm_destroy(&ar->entities);
    return 0;
}

int archive_create_item (struct archive* ar, const void* name, size_t name_bytes)
{
    if (ar->mode != WRITE || name_bytes > UINT16_MAX) {
        return -1;
    }

    struct archive_entity* entity = calloc(1, sizeof (struct archive_entity));
    if (entity == NULL) {
        return -1;
    }

    entity->header_start = ftell(ar->file_stream);

    uint16_t nbytes = name_bytes;
    if (fwrite(&nbytes, sizeof (nbytes), 1, ar->file_stream) != sizeof (nbytes)) {
        return -1;
    }

    if (fwrite(name, 1, name_bytes, ar->file_stream) != name_bytes) {
        return -1;
    }

    uint64_t content_bytes = 0;
    if (fwrite(&content_bytes, sizeof (content_bytes), 1, ar->file_stream) != sizeof (content_bytes)) {
        return -1;
    }
    entity->start = ftell(ar->file_stream);
    entity->length = 0;

    hm_set_copy(&ar->entities, name, name_bytes, entity);
}

int archive_write_item  (struct archive* ar, void* buffer, size_t buffer_bytes);
int archive_close_item  (struct archive* ar);

int archive_open_item  (struct archive* ar, const unsigned char* name, size_t name_bytes);
int archive_read_item  (struct archive* ar, void* buffer, size_t buffer_bytes);
int archive_close_item (struct archive* ar);