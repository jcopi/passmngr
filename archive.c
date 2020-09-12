#include "archive.h"

#include <string.h>

#define MIN(A,B)  ((A) > (B) ? (B) : (A))

archive_result_t archive_open (const char* file_name, archive_mode_t mode)
{
    archive_t ar = {0};
    archive_result_t result = {0};

    switch (mode) {
    case READ:
        ar.file = fopen(file_name, "rb");
        if (ar.file == NULL) {
            return ERR(result, E_FILE_OPEN_ERROR);
        }

        ar.open = true;

        empty_result_t er = archive_parse_index(&ar);
        if (IS_ERR(er)) {
            return ERR(result, er.result.error);
        }

        return OK(result, ar);
    case WRITE:
        ar.file = fopen(file_name, "wb");
        if (ar.file == NULL) {
            return ERR(result, E_FILE_OPEN_ERROR);
        }

        ar.open = true;

        return OK(result, ar);
    }
}
empty_result_t archive_close (archive_t* ar);

item_result_t archive_item_open (archive_t* ar, const uint8_t* name, uint16_t name_size)
{
    archive_item_t item = {0};
    item_result_t result = {0};

    if (!ar->open) {
        return ERR(result, U_ARCHIVE_NOT_OPEN);
    }
    if (ar->borrowed) {
        return ERR(result, U_ITEM_STILL_OPEN);
    }

    switch (ar->mode) {
    case READ:
        // Find the requested item from the item list
        size_result_t sr = archive_find_item_by_name(ar, name, name_size);
        if (IS_ERR(sr)) {
            return ERR(result, sr.result.error);
        }

        item.parent = ar;
        item.self = &ar->items[sr.result.value];
        item.current = 0;

        ar->borrowed = true;
        return OK(result, item);
    case WRITE:
        // Ensure the item hasn't been opened previously
        size_result_t sr = archive_find_item_by_name(ar, name, name_size);
        if (IS_OK(sr)) {
            return ERR(result, R_ITEM_PREV_OPENED);
        }

        // Add a new item to the list
        // return the new item
        size_t new_item_count = ar->item_count + 1;
        size_t new_names_size = ar->names_size + name_size;
        size_t prev_item_count = ar->item_count;
        size_t prev_names_size = ar->names_size;
        archive_item_info_t* tmp_info = realloc(ar->items, sizeof (archive_item_info_t) * new_item_count);
        uint8_t* tmp_names = realloc(ar->names_buffer, (sizeof (uint8_t) * new_names_size));
        if (tmp_info == NULL || tmp_names == NULL) {
            return ERR(result, R_ALLOCATION_ERROR);
        }

        ar->items = tmp_info;
        ar->item_count = new_item_count;
        ar->names_buffer = tmp_names;
        ar->names_size = new_names_size;
        memcpy(&ar->names_buffer[prev_names_size], name, name_size);
        ar->items[prev_item_count] = (archive_item_info_t){
            .name=&ar->names_buffer[prev_item_count],
            .name_size=name_size,
            .start=ar->item_content_size,
            .size=0,
        };

        item.parent = ar;
        item.self = &ar->items[prev_item_count];
        item.current = 0;

        ar->borrowed = true;
        return OK(result, item);
    }
}

size_result_t archive_item_read (archive_item_t* item, uint8_t* buffer, size_t buffer_size)
{
    size_t bytes_to_read = 0;
    size_result_t result = {0};

    if (item->parent->open == false) {
        return ERR(result, U_ARCHIVE_NOT_OPEN);
    }

    if (item->parent->mode != READ) {
        return ERR(result, U_INCORRECT_OP_MODE);
    }

    if (item->parent->borrowed == false) {
        return ERR(result, U_ITEM_NOT_OPEN);
    }

    size_t remaining_bytes = item->self->size - item->current;
    if (remaining_bytes == 0) {
        return ERR(result, R_END_OF_ITEM);
    }
    bytes_to_read = MIN(remaining_bytes, buffer_size);

    if (fread(buffer, sizeof (uint8_t), bytes_to_read, item->parent->file) != bytes_to_read) {
        return ERR(result, E_FILE_READ_ERROR);
    }

    return OK(result, bytes_to_read);
}
size_result_t archive_item_write (archive_item_t* item, const uint8_t* buffer, size_t buffer_size)
{
    size_t bytes_to_write = buffer_size;
    size_result_t result = {0};

    if (item->parent->open == false) {
        return ERR(result, U_ARCHIVE_NOT_OPEN);
    }

    if (item->parent->mode != READ) {
        return ERR(result, U_INCORRECT_OP_MODE);
    }

    if (item->parent->borrowed == false) {
        return ERR(result, U_ITEM_NOT_OPEN);
    }

    if (fwrite(buffer, sizeof (uint8_t), bytes_to_write, item->parent->file) != bytes_to_write) {
        fseek(item->parent->file, item->parent->item_content_size, SEEK_SET);
        return ERR(result, E_FILE_WRITE_ERROR);
    }

    item->parent->item_content_size += bytes_to_write;
    item->self->size += bytes_to_write;
    item->current += bytes_to_write;
    
    return OK(result, bytes_to_write);
}

empty_result_t archive_item_close (archive_item_t* item)
{
    empty_result_t result = {0};
    
    item->parent->borrowed = false;
    item->parent = NULL;
    item->self = NULL;
    
    return OK_EMPTY(result);
}

static info_result_t archive_read_info (archive_t* ar, size_t bytes_to_read)
{   
    // item info is stored as 4 fields, in the order name_length, name, start, size
    // they will be read one at a time
    archive_item_info_t info = {0};
    info_result_t result = {0};

    size_t bytes_remaining = bytes_to_read;

    if (bytes_remaining < sizeof (info.name_size)) {
        return ERR(result, F_INDEX_PARSE_ERROR);
    }

    if (fread(&info.name_size, sizeof (info.name_size), 1, ar->file) != 1) {
        return ERR(result, E_FILE_READ_ERROR);
    }
    bytes_remaining -= sizeof (info.name_size);

    if (bytes_remaining < sizeof (uint8_t) * info.name_size) {
        return ERR(result, F_INDEX_PARSE_ERROR);
    }

    // Allocate enough room on the archive item name buffer to hold this name
    size_t new_names_size = ar->names_size + info.name_size;
    size_t prev_names_size = ar->names_size;
    uint8_t* tmp = realloc(ar->names_buffer, sizeof (uint8_t) * new_names_size);
    if (tmp == NULL) {
        return ERR(result, R_ALLOCATION_ERROR);
    }
    ar->names_size = new_names_size;
    
    if (fread(&ar->names_buffer[prev_names_size], sizeof (uint8_t), info.name_size, ar->file) != info.name_size) {
        return ERR(result, E_FILE_READ_ERROR);
    }
    bytes_remaining -= sizeof (uint8_t) * info.name_size;

    if (bytes_remaining < sizeof (info.start) + sizeof (info.size)) {
        return ERR(result, F_INDEX_PARSE_ERROR);
    }


}

static empty_result_t archive_write_info (archive_t* ar, archive_item_info_t info);

empty_result_t archive_parse_index (archive_t* ar)
{
    empty_result_t result = {0};

    if (ar->open != true) {
        return ERR(result, U_ARCHIVE_NOT_OPEN);
    }

    if (ar->borrowed != true) {
        return ERR(result, U_ITEM_STILL_OPEN);
    }

    if (ar->mode != READ) {
        return ERR(result, U_INCORRECT_OP_MODE);
    }

    if (fseek(ar->file, -1 * sizeof (ar->index_size), SEEK_END) != 0) {
        return ERR(result, E_FILE_SEEK_ERROR);
    }

    if (fread(&ar->index_size, sizeof (ar->index_size), 1, ar->file) != 1) {
        return ERR(result, E_FILE_READ_ERROR);
    }

    size_t bytes_remaining = ar->index_size;
    for (; bytes_remaining > 0; ) {
        archive_item_info_t info = {0};

        if (bytes_remaining < sizeof (info.name_size)) {
            return ERR(result, F_INDEX_PARSE_ERROR);
        }

        if (fread(&info.name_size, sizeof (info.name_size), 1, ar->file) != 1) {
            return ERR(result, E_FILE_READ_ERROR);
        }
        bytes_remaining -= sizeof (info.name_size);

        if (bytes_remaining < sizeof (uint8_t) * info.name_size) {
            return ERR(result, F_INDEX_PARSE_ERROR);
        }

        // Allocate enough room on the archive item name buffer to hold this name
        size_t new_names_size = ar->names_size + info.name_size;
        size_t prev_names_size = ar->names_size;
        uint8_t* tmp = realloc(ar->names_buffer, sizeof (uint8_t) * new_names_size);
        if (tmp == NULL) {
            return ERR(result, R_ALLOCATION_ERROR);
        }
        ar->names_size = new_names_size;
        
        if (fread(&ar->names_buffer[prev_names_size], sizeof (uint8_t), info.name_size, ar->file) != info.name_size) {
            return ERR(result, E_FILE_READ_ERROR);
        }
        bytes_remaining -= sizeof (uint8_t) * info.name_size;

        if (bytes_remaining < sizeof (info.start) + sizeof (info.size)) {
            return ERR(result, F_INDEX_PARSE_ERROR);
        }

    }
}
empty_result_t archive_write_index (archive_t* ar);



size_result_t archive_find_item_by_name (archive_t* ar, const uint8_t* name, uint16_t name_size)
{
    size_t i;
    size_result_t result = {0};

    for (i = 0; i < ar->item_count; i++) {
        if (ar->items[i].name_size == name_size && memcmp(ar->items[i].name, name, name_size) == 0) {
            return OK(result, i);
        }
    }

    return ERR(result, R_ITEM_NOT_FOUND);
}

