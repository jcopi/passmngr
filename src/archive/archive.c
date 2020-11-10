#include <string.h>
#include <stdlib.h>

#include <archive.h>

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))

static archive_info_ref_result_t archive_read_info (archive_t* ar, size_t remaining_bytes);
static archive_size_result_t archive_write_info (archive_t* ar, archive_item_info_t* info);

static archive_empty_result_t archive_parse_index (archive_t* ar);
static archive_empty_result_t archive_write_index (archive_t* ar);

static archive_size_result_t archive_find_item (archive_t* ar, const byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes);

archive_result_t archive_open (const char* file_name, archive_mode_t mode)
{
    archive_t ar = {0};
    archive_result_t result = {0};

    switch (mode) {
    case READ:
    {
        ar.file = fopen(file_name, "rb");
        if (ar.file == NULL) {
            return ERR(result, FILE_OPEN_FAILED);
        }

        ar.opened = true;
        ar.mode   = READ;

        archive_empty_result_t er = archive_parse_index(&ar);
        if (IS_ERR(er)) {
            return ERR(result, UNWRAP_ERR(er));
        }

        return OK(result, ar);
    }
    case WRITE:
    {
        ar.file = fopen(file_name, "wb");
        if (ar.file == NULL) {
            return ERR(result, FILE_OPEN_FAILED);
        }

        ar.opened = true;
        ar.mode   = WRITE;

        return OK(result, ar);
    }
    default: return ERR(result, FATAL_UNEXPECTED);
    }
}

archive_empty_result_t archive_close (archive_t* ar)
{
    assert(ar->opened == true);
    assert(ar->locked == false);

    archive_empty_result_t result = {0};

    switch (ar->mode) {
    case READ: break;
    case WRITE:
    {
        archive_empty_result_t er = archive_write_index(ar);
        if (IS_ERR(er)) {
            return ERR(result, UNWRAP_ERR(er));
        }

        break;
    }
    }

    fclose(ar->file);

    for (size_t i = 0; i < ar->item_count; i++) {
        free(ar->items[i]);
    }
    free(ar->items);

    *ar = (archive_t) {0};

    return OK_EMPTY(result);
}

bool archive_has_item (archive_t* ar, const byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes)
{
    return IS_OK(archive_find_item(ar, name, name_bytes))
}

archive_item_result_t archive_item_open (archive_t* ar, const byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes)
{
    assert(ar->opened == true);
    assert(ar->locked == false);

    archive_item_t item = {0};
    archive_item_result_t result = {0};

    switch (ar->mode) {
    case READ:
    {
        assert(ar->indexed == true);
        archive_size_result_t sr = archive_find_item(ar, name, name_bytes);
        if (IS_ERR(sr)) {
            return ERR(result, UNWRAP_ERR(sr));
        }

        size_t i = UNWRAP(sr);
        
        item = (archive_item_t){
            .parent = ar,
            .info = ar->items[i],
            .current = 0
        };

        if (fseek(ar->file, item.info->start, SEEK_SET) != 0) {
            return ERR(result, FILE_SEEK_FAILED);
        }

        ar->locked = true;

        return OK(result, item);
    }
    case WRITE:
    {
        archive_size_result_t sr = archive_find_item(ar, name, name_bytes);
        if (IS_OK(sr)) {
            return ERR(result, RUNTIME_ITEM_PREV_OPENED);
        }

        archive_item_info_t* info = malloc(sizeof (archive_item_info_t) + name_bytes);
        if (info == NULL) {
            return ERR(result, FATAL_OUT_OF_MEMORY);
        }

        memcpy(info->name, name, name_bytes);
        info->name_bytes = name_bytes;
        info->start = ar->items_bytes;
        info->bytes = 0;

        size_t new_item_count = ar->item_count + 1;
        archive_item_info_t** tmp = realloc(ar->items, sizeof (*tmp) * new_item_count);
        if (tmp == NULL) {
            free(info);
            return ERR(result, FATAL_OUT_OF_MEMORY);
        }

        size_t i = ar->item_count;

        ar->items = tmp;
        ar->items[ar->item_count] = info;
        ar->item_count = new_item_count;

        item = (archive_item_t) {
            .parent = ar,
            .info = ar->items[i],
            .current = 0,
        };

        if (fseek(ar->file, 0, SEEK_END) != 0) {
            return ERR(result, FILE_SEEK_FAILED);
        }

        ar->locked = true;

        return OK(result, item);
    }
    default: return ERR(result, FATAL_UNEXPECTED);
    }
}

archive_size_result_t archive_item_read (archive_item_t* item, byte_t* buffer, size_t buffer_bytes)
{
    assert(item->parent->opened == true);
    assert(item->parent->mode == READ);
    assert(item->parent->locked == true);
    assert(item->parent->indexed == true);

    size_t bytes_to_read;
    archive_size_result_t result = {0};

    size_t bytes_remaining = item->info->bytes - item->current;
    bytes_to_read = MIN(bytes_remaining, buffer_bytes);

    if (fread(buffer, 1, bytes_to_read, item->parent->file) != bytes_to_read) {
        return ERR(result, FILE_READ_FAILED);
    }

    return OK(result, bytes_to_read);
}

archive_size_result_t archive_item_write (archive_item_t* item, const byte_t* buffer, size_t buffer_bytes)
{
    assert(item->parent->opened == true);
    assert(item->parent->mode == WRITE);
    assert(item->parent->locked == true);

    archive_size_result_t result = {0};

    if (fwrite(buffer, 1, buffer_bytes, item->parent->file) != buffer_bytes) {
        return ERR(result, FILE_WRITE_FAILED);
    }

    item->current += buffer_bytes;
    item->info->bytes += buffer_bytes;
    item->parent->items_bytes += buffer_bytes;

    return OK(result, buffer_bytes);
}
archive_empty_result_t archive_item_close (archive_item_t* item)
{
    assert(item->parent->opened == true);
    assert(item->parent->locked == true);

    archive_empty_result_t result = {0};

    item->parent->locked = false;

    if (fflush(item->parent->file) != 0) {
        return ERR(result, FILE_WRITE_FAILED);
    }

    item->parent = NULL;
    item->info = NULL;
    item->current = 0;

    return OK_EMPTY(result);
}

static archive_info_ref_result_t archive_read_info (archive_t* ar, size_t remaining_bytes)
{
    assert(ar->opened == true);
    assert(ar->mode == READ);
    assert(ar->locked == false);

    archive_info_ref_result_t result = {0};
    COMMON_ITEM_NAME_TYPE name_bytes = 0;

    if (remaining_bytes <= sizeof (name_bytes)) {
        return ERR(result, FATAL_INDEX_MALFORMED);
    }

    if (fread(&name_bytes, sizeof (name_bytes), 1, ar->file) != 1) {
        return ERR(result, FILE_READ_FAILED);
    }
    remaining_bytes -= sizeof (name_bytes);

    if (remaining_bytes < name_bytes + sizeof (ITEM_START_TYPE) + sizeof (ITEM_SIZE_TYPE)) {
        return ERR(result, FATAL_INDEX_MALFORMED);
    }

    archive_item_info_t* info = malloc(sizeof (archive_item_info_t) + name_bytes);
    if (info == NULL) {
        return ERR(result, FATAL_OUT_OF_MEMORY);
    }

    info->name_bytes = name_bytes;
    if (fread(info->name, 1, info->name_bytes, ar->file) != info->name_bytes) {
        free(info);
        return ERR(result, FILE_READ_FAILED);
    }
    if (fread(&info->start, sizeof (info->start), 1, ar->file) != 1) {
        free(info);
        return ERR(result, FILE_READ_FAILED);
    }
    if (fread(&info->bytes, sizeof (info->bytes), 1, ar->file) != 1) {
        free(info);
        return ERR(result, FILE_READ_FAILED);
    }

    return OK(result, info);
}

static archive_size_result_t archive_write_info (archive_t* ar, archive_item_info_t* info)
{
    assert(ar->opened == true);
    assert(ar->mode == WRITE);
    assert(ar->locked == false);

    size_t bytes_written = 0;
    archive_size_result_t result = {0};

    if (fwrite(&info->name_bytes, sizeof (info->name_bytes), 1, ar->file) != 1) {
        return ERR(result, FILE_WRITE_FAILED);
    }
    bytes_written += sizeof (info->name_bytes);

    if (fwrite(info->name, 1, info->name_bytes, ar->file) != info->name_bytes) {
        return ERR(result, FILE_WRITE_FAILED);
    }
    bytes_written += info->name_bytes;

    if (fwrite(&info->start, sizeof (info->start), 1, ar->file) != 1) {
        return ERR(result, FILE_WRITE_FAILED);
    }
    bytes_written += sizeof (info->start);

    if (fwrite(&info->bytes, sizeof (info->bytes), 1, ar->file) != 1) {
        return ERR(result, FILE_WRITE_FAILED);
    }
    bytes_written += sizeof (info->bytes);

    return OK(result, bytes_written);
}

// parse_index will seek to the end of the file, read the total index size, then read the index.
static archive_empty_result_t archive_parse_index (archive_t* ar)
{
    // The conditions covered by the assertions are usage error
    assert(ar->opened == true);
    assert(ar->mode == READ);
    assert(ar->locked == false);
    assert(ar->indexed == false);

    ar->item_count = 0;

    archive_empty_result_t result = {0};

    if (fseek(ar->file, -1 * sizeof (ar->index_bytes), SEEK_END) != 0) {
        return ERR(result, FILE_SEEK_FAILED);
    }

    if (fread(&ar->index_bytes, sizeof (ar->index_bytes), 1, ar->file) != 1) {
        return ERR(result, FILE_READ_FAILED);
    }

    if (fseek(ar->file, -1 * (ar->index_bytes + sizeof (ar->index_bytes)), SEEK_END) != 0) {
        return ERR(result, FILE_SEEK_FAILED);
    }

    size_t bytes_remaining = ar->index_bytes;
    while (bytes_remaining > 0) {
        archive_info_ref_result_t ir = archive_read_info(ar, bytes_remaining);
        if (IS_ERR(ir)) {
            return ERR(result, UNWRAP_ERR(ir));
        }

        archive_item_info_t* info = UNWRAP(ir);
        bytes_remaining -= sizeof (info->start) + sizeof (info->bytes) + sizeof (info->name_bytes) + info->name_bytes;

        size_t new_item_count = ar->item_count + 1;
        archive_item_info_t** tmp = realloc(ar->items, sizeof (*tmp) * new_item_count);
        if (tmp == NULL) {
            free(info);
            return ERR(result, FATAL_OUT_OF_MEMORY);
        }

        ar->items = tmp;
        ar->items[ar->item_count] = info;
        ar->item_count = new_item_count;
    }

    ar->indexed = true;
    return OK_EMPTY(result);
}
static archive_empty_result_t archive_write_index (archive_t* ar)
{
    assert(ar->opened == true);
    assert(ar->mode == WRITE);
    assert(ar->locked == false);

    archive_empty_result_t result = {0};
    size_t bytes_written = 0;

    if (fseek(ar->file, 0, SEEK_END) != 0) {
        return ERR(result, FILE_SEEK_FAILED);
    }

    for (size_t i = 0; i < ar->item_count; i++) {
        archive_size_result_t sr = archive_write_info(ar, ar->items[i]);
        if (IS_ERR(sr)) {
            return ERR(result, UNWRAP_ERR(sr));
        }

        bytes_written += UNWRAP(sr);
    }

    ar->index_bytes = bytes_written;

    if (fwrite(&ar->index_bytes, sizeof (ar->index_bytes), 1, ar->file) != 1) {
        return ERR(result, FILE_WRITE_FAILED);
    }

    return OK_EMPTY(result);
}

static archive_size_result_t archive_find_item (archive_t* ar, const byte_t* name, COMMON_ITEM_NAME_TYPE name_bytes)
{
    assert(ar->opened == true);
    // assert(ar->indexed == true);
    assert(ar->locked == false);
    // assert(ar->mode == READ);

    size_t i;
    archive_size_result_t result = {0};

    for (i = 0; i < ar->item_count; i++) {
        if (ar->items[0]->name_bytes == name_bytes && memcmp(ar->items[i]->name, name, name_bytes) == 0) {
            return OK(result, i);
        }
    }

    return ERR(result, RUNTIME_ITEM_NOT_FOUND);
}
