#include "archive.h"

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
        item.selt = &ar->items[sr.result.value];
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
        
        break;
    }
}
size_result_t archive_item_read (archive_item_t* item, uint8_t* buffer, size_t buffer_size);
size_result_t archive_item_write (archive_item_t* item, const uint8_t* buffer, size_t buffer_size);
empty_result_t archive_item_close (archive_item_t* item);

info_result_t archive_read_info (archive_t* ar);
empty_result_t archive_write_info (archive_t* ar, archive_item_info_t info);

empty_result_t archive_parse_index (archive_t* ar);
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

