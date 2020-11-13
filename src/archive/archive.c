#include <archive.h>
#include <assert.h>

ar_empty_result_t  archive_parse_index (archive_t* ar);
ar_record_result_t archive_read_record (archive_t* ar);

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

        ar_empty_result_t maybe_parsed = archive_parse_index(ar);
        if (IS_ERR(maybe_parsed)) {
            return ERR(result, UNWRAP_ERR(maybe_parsed));
        }

        return OK_EMPTY(result);
    case WRITE:
        break;
    default:
        return ERR(result, AR_INVALID_MODE);
    }
}

ar_empty_result_t archive_close (archive_t* ar);

ar_item_result_t  archive_item_open   (archive_t* ar, const byte_t* const name, const size_t name_size);
ar_empty_result_t archive_item_close  (archive_item_t* item);
ar_size_result_t  archive_item_read   (archive_item_t* item, const byte_t* buffer, const size_t buffer_size);
ar_size_result_t  archive_item_write  (archive_item_t* item, const byte_t* const buffer, const size_t buffer_size);
ar_empty_result_t archive_item_delete (archive_item_t* item);
ar_empty_result_t archive_item_splice (archive_item_t* item, const uint64_t start, const uint64_t size);

bool archive_has_item (archive_t* ar, const byte_t* const name, const size_t name_size);
