#include <string.h>

#include "vault.h"

#define ASSERT_FILE_MODE(v,m) if (v->mode == m) return VAULT_EBADFILEMODE

int vault_open (vault_t* v, const char* file_name, enum vault_open_mode mode)
{
    memset(v, 0, sizeof (vault_t));

    v->file_stream = fopen(file_name, (mode == READ ? "rb" : "wb"));
    if (v->file_stream == NULL) {
        return VAULT_EOPENFAIL;
    }
    
    return vault_next(v);
}

int vault_next (vault_t* v)
{
    // Seek to the next item and parse the header
    unsigned long position = ftell(v->file_stream);
    if (position != v->item_next_position) {
        position = v->item_next_position;
        if (!fseek(v->file_stream, position, SEEK_SET)) {
            return VAULT_ESEEKFAIL;
        }
    }

    unsigned char buffer[VAULT_HEADER_SIZE];
    if (fread(&buffer, 1, VAULT_HEADER_SIZE, v->file_stream) != VAULT_HEADER_SIZE) {
        return VAULT_EREADFAIL;
    }

    memcpy(&v->current_header.magic_number, &buffer[VAULT_HEADER_MAGIC_NUMBER_OFFSET], VAULT_HEADER_MAGIC_NUMBER_SIZE);
    memcpy(&v->current_header.item_name, &buffer[VAULT_HEADER_NAME_OFFSET], VAULT_HEADER_NAME_SIZE);
    memcpy(&v->current_header.item_length, &buffer[VAULT_HEADER_ITEM_LENGTH_OFFSET], VAULT_HEADER_ITEM_LENGTH_SIZE);

    if (v->current_header.magic_number != VAULT_HEADER_MAGIC_NUMBER) {
        return VAULT_EINVALIDFILE;
    }

    v->item_header_position = position;
    v->item_start_position = position + VAULT_HEADER_SIZE;
    v->item_next_position = v->item_start_position + v->current_header.item_length;
    v->current_position = ftell(v->file_stream);

    return VAULT_ESUCCESS;
}

int vault_read_header (vault_t* v, vault_header_t* header)
{
    // ASSERT_FILE_MODE(v, READ);
    
    memcpy(header, &v->current_header, sizeof (vault_header_t));
    return VAULT_ESUCCESS;
}

int vault_write_new_item (vault_t* v, char* item_name) {
    ASSERT_FILE_MODE(v, WRITE);
    
    size_t name_length = strlen(item_name);
    if (name_length >= VAULT_HEADER_NAME_SIZE) {
        return VAULT_EFAILURE;
    }
    
    // Write values into the vault_t->current_header and write the data to disk
    v->current_header.magic_number = VAULT_HEADER_MAGIC_NUMBER;
    memcpy(&v->current_header.item_name, item_name, name_length);
    v->current_header.item_length = 0;
    
    unsigned char buffer[VAULT_HEADER_SIZE];
    memcpy(&buffer[VAULT_HEADER_MAGIC_NUMBER_OFFSET], &v->current_header.magic_number, VAULT_HEADER_MAGIC_NUMBER_SIZE);
    memcpy(&buffer[VAULT_HEADER_NAME_OFFSET], &v->current_header.item_name, VAULT_HEADER_NAME_SIZE);
    memcpy(&buffer[VAULT_HEADER_ITEM_LENGTH_OFFSET], &v->current_header.item_length, VAULT_HEADER_ITEM_LENGTH_SIZE);
    
    v->item_header_position = ftell(v->file_stream);
    
    if (fwrite(&buffer, 1, VAULT_HEADER_SIZE, v->file_stream) != VAULT_HEADER_SIZE) {
        return VAULT_EWRITEFAIL;
    }
    
    // Set the file positions of items related to the current item within vault_t
    v->item_start_position = ftell(v->file_stream);
    v->item_next_position = v->item_start_position;
    
    v->current_position = v->item_start_position;
    
    return VAULT_ESUCCESS;
}

int vault_read_data (vault_t* v, void* buffer, size_t buffer_length, size_t* length_read)
{
    ASSERT_FILE_MODE(v, READ);
    
    unsigned long remaining = v->item_next_position - v->current_position;
    size_t read_length = (remaining >= buffer_length ? buffer_length : remaining);
    
    // Check if eof was reached
    *length_read = fread(buffer, 1, read_length, v->file_stream);
    if (*length_read != read_length) {
        // Even in the event of eof, read_length bytes should always be able to be read
        return VAULT_EREADFAIL;
    }

    return VAULT_ESUCCESS;
}

int vault_write_data (vault_t* v, void* buffer, size_t buffer_length, size_t* length_written)
{
    // Writing data is more complicated than reading it, as it requires the header to be updates after each successful write
    // This will write data, then seek to the header item_length location, write the next item length, then seek back to the end of the file.
    ASSERT_FILE_MODE(v, WRITE);
    
    *length_written = fwrite(buffer, 1, buffer_length, v->file_stream);
    v->current_header.item_length += *length_written;
    
    unsigned long current = ftell(v->file_stream);
    if (!fseek(v->file_stream, v->item_header_position + VAULT_HEADER_ITEM_LENGTH_OFFSET, SEEK_SET)) {
        return VAULT_ESEEKFAIL;
    }
    if (fwrite(&v->current_header.item_length, 1, VAULT_HEADER_ITEM_LENGTH_SIZE, v->file_stream) != VAULT_HEADER_ITEM_LENGTH_SIZE) {
        return VAULT_EWRITEFAIL;
    }
    if (!fseek(v->file_stream, current, SEEK_SET)) {
        return VAULT_ESEEKFAIL;
    }
    
    if (*length_written != buffer_length) {
        return VAULT_EWRITEFAIL;
    }
    
    return VAULT_ESUCCESS;
    
}

int vault_skip_until (vault_t* v, const char* item_name) {
    return VAULT_EFAILURE;
}

struct vault_return init_vault (struct vault* v)
{
    memset(v, 0, sizeof (struct vault));
    v->buffer = sodium_allocarray(VAULT_BLOCKBYTES, 1);
    if (v->buffer == NULL) {
        v->state = ERROR;
        return R_ERROR(VAULT_ALLOCATION_FAILURE);
    }

    v->state = INIT;
    return R_SUCCESS;
}

struct vault_return vault_create (struct vault* v, const char* name, const char* password, size_t password_length)
{
    // If this vault is in use it can't be used
    if (v->is_open || v->fstream != NULL || v->state != INIT) {
        return R_ERROR(VAULT_ALREADY_OPEN);
    }
    
    v->fstream = fopen(name, "wb");
    if (v->fstream == NULL) {
        v->state = ERROR;
        return R_ERROR(VAULT_OPEN_FAILURE);
    }

    v->is_open = true;
    v->state = OPEN;
    v->mode = WRITE;

    // Vault file is open, now a master salt needs to be generated and written to disk.
    if (sodium_init() != 0) {
        fclose(v->fstream);
        v->state = ERROR;
        return R_ERROR(VAULT_ALLOCATION_FAILURE);
    }
    
    randombytes_buf(v->salt, sizeof (v->salt));
    if (fwrite(v->salt, 1, sizeof (v->salt), v->stream) != sizeof (v->salt)) {
        fclose(v->fstream);
        v->state = ERROR;
        return R_ERROR(VAULT_WRITE_FAILURE);
    }
    
    // Create a master key record
    v->master_key = sodium_allocarray(VAULT_KEYBYTES, 1);
    if (v->master_key == NULL) {
        fclose(v->fstream);
        v->state = ERROR;
        return R_ERROR(VAULT_ALLOCATION_FAILURE);
    }

    if (crypto_pwhash(v->master_key, VAULT_KEYBYTES, password, password_length, v->salt,
        crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        fclose(v->fstream);
        sodium_free(v->master_key);
        v->master_key = NULL;
        v->state = ERROR;
        return R_ERROR(VAULT_ALLOCATION_FAILURE);
    }

    unsigned char keys_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_init_push(&v->keys_state, keys_header, v->master_key);
    if (fwrite(header, 1, sizeof (header), v->fstream) != sizeof (header)) {
        fclose(v->fstream);
        sodium_memzero(v->master_key, VAULT_KEYBYTES);
        sodium_free(v->master_key);
        v->master_key = NULL;
        v->state = ERROR;
        return R_ERROR(VAULT_WRITE_FAILURE);
    }

    // TODO: handle errors
    sodium_mprotect_noaccess(v->master_key);
    v->state = READY_FOR_KEYS;

    return R_SUCCESS;
}


struct vault_return vault_add_item (struct vault* v, const char* item_name)
{
    // If this vault is in use it can't be used
    if (!v->is_open || v->fstream == NULL || v->state != READY_FOR_KEYS) {
        v->state = ERROR;
        return R_ERROR(VAULT_INVALID_STATE);
    }

    size_t name_len = strlen(item_name);
    if (name_len >= VAULT_ITEMNAME_MAXBYTES) {
        v->state = ERROR;
        return R_ERROR(VAULT_INVALID_ITEM);
    }

    // size_t entry_length = sizeof (uint16_t) + name_len + VAULT_KEYBYTES;
    uint16_t name_bytes = name_len;
    size_t bytes_to_write = sizeof (name_bytes);
    // Write the key entries to the guarded allocation buffer.
    // If the buffer is full encrypt and write it to disk. 
    // *The key entry could potentially by larger than the buffer.
    size_t remaining = VAULT_BLOCKBYTES - v->content_bytes;
    // Attempt to write the 2 byte name length
    #define MIN(A,B) ((A) < (B) ? (A) : (B))

    do {
        size_t amount_written = MIN(remaining, bytes_to_write);
        memcpy(&v->buffer[v->content_bytes], &name_bytes, amount_written);
        bytes_to_write -= amount_written;
        v->content_bytes += amount_written;

        remaining = VAULT_BLOCKBYTES - v->content_bytes;
        if (remaining == 0) {
            // Encrypt and write to disk
            sodium_mprotect_readonly(v->master_key);
            unsigned char out_buff[VAULT_BLOCKBYTES + crypto_secretstream_xchacha20poly1305_ABYTES];
            unsigned long long enc_len;
            if (crypto_secretstream_xchacha20poly1305_push(&v->keys_state, out_buff, &enc_len, v->buffer, VAULT_BLOCKBYTES,NULL, 0, 0) != 0) {
                // Failed to encrypt
                fclose(v->fstream);
                sodium_memzero(v->master_key, VAULT_KEYBYTES);
                sodium_memzero(v->buffer, VAULT_BLOCKBYTES);
                sodium_free(v->master_key);
                v->master_key = NULL;
                v->state = ERROR;
                return R_ERROR(VAULT_INVALID_ITEM);
            }
            if (fwrite(out_buff, 1, (size_t) enc_len, v->fstream) != (size_t) enc_len) {
                
            }
        }
    } while (bytes_to_write);

    #undef MIN
}

struct vault_return vault_commit_items (struct vault* v);
struct vault_return vault_start_item   (struct vault* v, const char* item_name);
struct vault_return vault_write_item   (struct vault* v, void* buffer, size_t buffer_size);
struct vault_return vault_commit_item  (struct vault* v);

struct vault_return vault_open       (struct vault* v, const char* name, const char* password, size_t password_length);
struct vault_return vault_open_item  (struct vault* v, const char* item_name);
struct vault_return vault_read_item  (struct vault* v, void* buffer, size_t buffer_size);
struct vault_return vault_close_item (struct vault* v);