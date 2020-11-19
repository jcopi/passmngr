#include <archive.h>
#include <assert.h>

#include <stdlib.h>

ar_empty_result_t  archive_parse_index  (archive_t* ar);
ar_empty_result_t  archive_create_index (archive_t* ar);
ar_item_result_t   archive_create_item  (archive_t* ar, const byte_t* const name, const size_t name_size);
ar_record_result_t archive_read_record  (archive_t* ar);

ar_empty_result_t archive_open (archive_t* ar, const char* file_name, const file_mode_t mode)
{
    ar_empty_result_t result = {0};

    *ar = (archive_t){0};

    switch (mode) {
    case READ:
        ar->file = fopen(file_name, "rb");
        if (ar->file == NULL) {
            return ERR(result, AR_FILE_OPEN_FAILED);
        }

        ar->opened = true;
        ar->mode = mode;
        
        hm_empty_result_t maybe_inited = hm_init(&ar->index);
        if (IS_ERR(maybe_inited)) {
            return ERR(result, UNWRAP_ERR(maybe_inited));
        }
        if (!WILL_FIT_IN_MAP(uint64_t)) {
            hm_set_deallocator(&ar->index, free);
        }

        ar_empty_result_t maybe_parsed = archive_parse_index(ar);
        if (IS_ERR(maybe_parsed)) {
            return ERR(result, UNWRAP_ERR(maybe_parsed));
        }

        return OK_EMPTY(result);
    case WRITE:
        ar->file = fopen(file_name, "wb");
        if (ar->file == NULL) {
            return ERR(result, AR_FILE_OPEN_FAILED);
        }

        ar->opened = true;
        ar->mode = mode;

        hm_empty_result_t maybe_inited = hm_init(&ar->index);
        if (IS_ERR(maybe_inited)) {
            return ERR(result, UNWRAP_ERR(maybe_inited));
        }
        if (!WILL_FIT_IN_MAP(uint64_t)) {
            hm_set_deallocator(&ar->index, free);
        }

        ar_empty_result_t maybe_written = archive_create_index(ar);
        if (IS_ERR(maybe_written)) {
            return ERR(result, UNWRAP_ERR(maybe_written));
        }

        return OK_EMPTY(result);
    default:
        return ERR(result, AR_INVALID_MODE);
    }
}

ar_empty_result_t archive_close (archive_t* ar)
{
    assert(ar->opened);
    assert(!ar->borrowed);

    ar_empty_result_t result = {0};

    fclose(ar->file);
    hm_destroy(&ar->index);

    ar->opened = false;

    return OK_EMPTY(result);
}

ar_item_result_t archive_item_open (archive_t* ar, const byte_t* const name, const size_t name_size)
{
    assert(ar->opened);
    assert(!ar->borrowed);

    ar_item_result_t result = {0};
    archive_item_t item = {0};

    switch (ar->mode) {
    case READ:
        hm_value_result_t maybe_index = hm_get(&ar->index, name, name_size);
        if (IS_ERR(maybe_index) && UNWRAP_ERR(maybe_index) == MAP_KEY_NOT_FOUND) {
            return ERR(result, AR_ITEM_NOT_FOUND);
        } else if (IS_ERR(maybe_index)) {
            return ERR(result, UNWRAP_ERR(maybe_index));
        }

        ar->borrowed = true;

        uint64_t index = UNWRAP(maybe_index);
        item = (archive_item_t){.parent = ar, .first_entry_start = index, .current_entry_start = index, .bytes_read = 0};
        return OK(result, item);
        
    case WRITE:
        hm_value_result_t maybe_index = hm_get(&ar->index, name, name_size);
        if (IS_ERR(maybe_index) && UNWRAP_ERR(maybe_index) == MAP_KEY_NOT_FOUND) {
            return archive_create_item(ar, name, name_size);
        } else if (IS_ERR(maybe_index)) {
            return ERR(result, UNWRAP_ERR(maybe_index));
        }

        // open the item and seek to the start of it
        item = (archive_item_t){.parent = ar, .first_entry_start = index, .current_entry_start = index, .bytes_read = 0};
        

        ar->borrowed = true;        

    default: return ERR(result, AR_INVALID_MODE);
    }
}

ar_empty_result_t archive_item_close  (archive_item_t* item);
ar_size_result_t  archive_item_read   (archive_item_t* item, const byte_t* buffer, const size_t buffer_size);
ar_size_result_t  archive_item_write  (archive_item_t* item, const byte_t* const buffer, const size_t buffer_size);
ar_empty_result_t archive_item_delete (archive_item_t* item);
ar_empty_result_t archive_item_splice (archive_item_t* item, const uint64_t start, const uint64_t size);

bool archive_has_item (archive_t* ar, const byte_t* const name, const size_t name_size);
