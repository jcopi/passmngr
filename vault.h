#ifndef _PASSMNGR_VAULT_
#define _PASSMNGR_VAULT_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <sodium.h>

#define VAULT_SALTBYTES         (crypto_pwhash_SALTBYTES)
#define VAULT_KEYBYTES          (crypto_secretstream_xchacha20poly1305_KEYBYTES)
#define VAULT_ITEMNAME_MAXBYTES (64)
#define VAULT_BLOCKBYTES        (2048)

/*
VAULT FORMAT
---------------------------------------------------------
| VAULT_SALTBYTES Bytes: Users master salt              |
---------------------------------------------------------
| 8 Bytes: Keys Section Byte Length | 2 Bytes: Item Cnt |
---------------------------------------------------------
  ENCRYPTED w/ Key Derived from Password + Master Salt
---------------------------------------------------------
| 2 Bytes: Item Name Length | N Bytes: Item Name        |
| ----------------------------------------------------- |
| VAULT_KEYBYTES Bytes: Item Key                        |
| ----------------------------------------------------- |
| ...                                                   |
---------------------------------------------------------
  ENCRYPTED w/ Key defined in Keys Section               
---------------------------------------------------------
| N Bytes: Item Content                                 |
---------------------------------------------------------
| ----------------------------------------------------- |
| 8 Bytes: Item Start Byte  | 8 Bytes: Item Byte Length |
| ----------------------------------------------------- |

The user salt will be read from the file, then the master
key will be generated. The keys section will be decrypted
into a gaurded allocation.

Items in Guarded Allocations:
 - User Password    (32B?) // This should be allocated outside the vault code
 - User Derived Key (32B)
 - Item Headers     (sizeof (struct item_header) * N items)
 - 1 Block for use in crypto_secretstream (2048B)

Vaults are read-only, changes require rewriting the entire vault
*/

enum vault_error {
    VAULT_SUCCESS,
    VAULT_ALREADY_OPEN,
    VAULT_WRONG_MODE,
    VAULT_READ_FAILURE,
    VAULT_WRITE_FAILURE,
    VAULT_OPEN_FAILURE,
    VAULT_ALLOCATION_FAILURE,
    VAULT_INVALID_STATE,
    VAULT_INVALID_ITEM,
    VAULT_CORRUPT
};

enum vault_mode {
    READ,
    WRITE
};

enum vault_state {
    NIL,
    INIT,
    OPEN,
    READY_FOR_KEYS,
    READY_FOR_CONTENT,
    ERROR
};

struct vault {
    bool is_open;
    enum vault_mode mode;
    enum vault_state state;
    FILE* fstream;

    unsigned char salt[VAULT_SALTBYTES];
    unsigned char* master_key;
    crypto_secretstream_xchacha20poly1305_state keys_state;

    struct item_header* items;
    size_t item_count;
    size_t current_item;
    crypto_secretstream_xchacha20poly1305_state item_state;

    unsigned char* buffer;
    size_t content_bytes;
};

struct item_header {
    char     name[VAULT_ITEMNAME_MAXBYTES];
    uint8_t  key[VAULT_KEYBYTES];
    uint64_t item_start;
    uint64_t item_length;
};

struct vault_return {
    bool error;
    union {
        enum vault_error err;
        size_t s;
    } value;
};

#define R_ERROR(E)  ((struct vault_return){.error=true,  .value.err=(E)})
#define R_SUCCESS   ((struct vault_return){.error=false, .value.err=VAULT_SUCCESS})
#define R_SIZE_T(S) ((struct vault_return){.error=false, .value.s=(S)})

struct vault_return init_vault (struct vault* v);

struct vault_return vault_create       (struct vault* v, const char* name, const char* password, size_t password_length);
struct vault_return vault_add_item     (struct vault* v, const char* item_name);
struct vault_return vault_clear_items  (struct vault* v);
struct vault_return vault_commit_items (struct vault* v);
struct vault_return vault_start_item   (struct vault* v, const char* item_name);
struct vault_return vault_write_item   (struct vault* v, void* buffer, size_t buffer_size);
struct vault_return vault_commit_item  (struct vault* v);

struct vault_return vault_open       (struct vault* v, const char* name, const char* password, size_t password_length);
struct vault_return vault_open_item  (struct vault* v, const char* item_name);
struct vault_return vault_read_item  (struct vault* v, void* buffer, size_t buffer_size);
struct vault_return vault_close_item (struct vault* v);

#endif
