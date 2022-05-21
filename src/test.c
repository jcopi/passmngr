#include <stdio.h>
#include <string.h>

#include <kv.h>


struct {struct{char* data; size_t length;} key; struct{char* data; size_t length;} value;} test_items[20] = {
    {.key={.data="test/keys/key0", .length=14}, .value={.data="value0", .length=6}},
    {.key={.data="test/keys/key1", .length=14}, .value={.data="value1", .length=6}},
    {.key={.data="test/keys/key2", .length=14}, .value={.data="value2", .length=6}},
    {.key={.data="test/keys/key3", .length=14}, .value={.data="value3", .length=6}},
    {.key={.data="test/keys/key4", .length=14}, .value={.data="value4", .length=6}},
    {.key={.data="test/keys/key5", .length=14}, .value={.data="value5", .length=6}},
    {.key={.data="test/keys/key6", .length=14}, .value={.data="value6", .length=6}},
    {.key={.data="test/keys/key7", .length=14}, .value={.data="value7", .length=6}},
    {.key={.data="test/keys/key8", .length=14}, .value={.data="value8", .length=6}},
    {.key={.data="test/keys/key9", .length=14}, .value={.data="value9", .length=6}},
    {.key={.data="test/keys/key10", .length=15}, .value={.data="value10", .length=7}},
    {.key={.data="test/keys/key11", .length=15}, .value={.data="value11", .length=7}},
    {.key={.data="test/keys/key12", .length=15}, .value={.data="value12", .length=7}},
    {.key={.data="test/keys/key13", .length=15}, .value={.data="value13", .length=7}},
    {.key={.data="test/keys/key14", .length=15}, .value={.data="value14", .length=7}},
    {.key={.data="test/keys/key15", .length=15}, .value={.data="value15", .length=7}},
    {.key={.data="test/keys/key16", .length=15}, .value={.data="value16", .length=7}},
    {.key={.data="test/keys/key17", .length=15}, .value={.data="value17", .length=7}},
    {.key={.data="test/keys/key18", .length=15}, .value={.data="value18", .length=7}},
    {.key={.data="test/keys/key19", .length=15}, .value={.data="value19", .length=7}}
};

#define countof(A) (sizeof(A) / sizeof((A)[0]))

char* const master_password = "fillip-barratry-skeptic-niggle-wardrobe-feed-pastime-paranoia";

#define DB_FILENAME ("./vault.db")


void print_slice_hex(slice s) {
    for (size_t i = 0; i < s.len; i++) {
        printf("%02x", ((uint8_t*)s.ptr)[i]);
    }
}

void print_slice_as_string(slice s) {
    for (size_t i = 0; i < s.len; i++) {
        printf("%c", ((char*)s.ptr)[i]);
    }
}

void print_item(void* ctx, guarded_t key, guarded_t value) {
    if (ctx != NULL) return;
    print_slice_as_string(key);
    printf(": \"");
    print_slice_as_string(value);
    printf("\"\n");
}

int main() {
    size_t master_password_length = strlen(master_password);

    nil_result_t nr = init_vault();
    if (IS_ERROR(nr)) {
        printf("Failed to initialize libsodium\n");
        return 1;
    }

    printf("Testing empty vault initialization\n");
    vault_result_t v_result = new_vault(DB_FILENAME, slice_ptr(master_password, master_password_length, master_password_length));
    if (IS_ERROR(v_result)) {
        printf("\tFailed: returned an error code [%i]\n", v_result.sum.error);

        printf("%s\n", sqlite3_errstr(v_result.sum.error));
        return 1;
    }
    if (IS_NULL(v_result)) {
        printf("\tFailed: returned a null result\n");
        return 1;
    }
    printf("\tPassed\n");

    vault_t v = UNWRAP(v_result);

    printf("Testing setting keys\n");
    for (size_t i = 0; i < countof(test_items); i++) {
        slice item_key = slice_ptr(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        slice item_value = slice_ptr(test_items[i].value.data, test_items[i].value.length, test_items[i].value.length);
        nil_result_t n_result = vault_set(&v, item_key, item_value);
        if (IS_ERROR(n_result)) {
            printf("\tFailed: returned an error code [%i]\n", n_result.sum.error);
            printf("%s\n", sqlite3_errstr(n_result.sum.error));
            return 1;
        }
    }
    printf("\tPassed\n");

    printf("Testing getting keys\n");
    for (size_t i = 0; i < countof(test_items); i++) {
        slice item_key = slice_ptr(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        slice item_value = slice_ptr(test_items[i].value.data, test_items[i].value.length, test_items[i].value.length);
        guarded_result_t s_result = vault_get(&v, item_key);
        if (IS_ERROR(s_result)) {
            printf("\tFailed: returned an error code [%i]\n", s_result.sum.error);
            printf("%s\n", sqlite3_errstr(s_result.sum.error));
            return 1;
        }
        if (IS_NULL(s_result)) {
            printf("\tFailed: returned a null result\n");
            return 1;
        }

        slice s = UNWRAP(s_result);

        if (s.len != item_value.len || memcmp(s.ptr, item_value.ptr, s.len) != 0) {
            printf("\tFailed: returned an invalid value\n");
            free_value(s);
            return 1;
        }

        free_value(s);
    }
    printf("\tPassed\n");

    destroy_vault(&v);

    printf("Testing existing vault initialization\n");
    v_result = new_vault(DB_FILENAME, slice_ptr(master_password, master_password_length, master_password_length));
    if (IS_ERROR(v_result)) {
        printf("\tFailed: returned an error code [%i]\n", v_result.sum.error);

        printf("%s\n", sqlite3_errstr(v_result.sum.error));
        return 1;
    }
    if (IS_NULL(v_result)) {
        printf("\tFailed: returned a null result\n");
        return 1;
    }
    printf("\tPassed\n");

    v = UNWRAP(v_result);

    printf("Testing getting keys from existing\n");
    for (size_t i = 0; i < countof(test_items); i++) {
        slice item_key = slice_ptr(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        slice item_value = slice_ptr(test_items[i].value.data, test_items[i].value.length, test_items[i].value.length);
        guarded_result_t s_result = vault_get(&v, item_key);
        if (IS_ERROR(s_result)) {
            printf("\tFailed: returned an error code [%i]\n", s_result.sum.error);
            printf("%s\n", sqlite3_errstr(s_result.sum.error));
            return 1;
        }
        if (IS_NULL(s_result)) {
            printf("\tFailed: returned a null result for key [%s]\n", (char*)item_key.ptr);
            return 1;
        }

        slice s = UNWRAP(s_result);

        if (s.len != item_value.len || memcmp(s.ptr, item_value.ptr, s.len) != 0) {
            printf("\tFailed: returned an invalid value\n");
            free_value(s);
            return 1;
        }

        free_value(s);
    }
    printf("\tPassed\n");

    printf("Testing all key iterator\n");
    nr = vault_iter(&v, (slice){0}, print_item, NULL);
    if (IS_ERROR(nr)) {
        printf("\tFailed: returned an error code [%i]\n", nr.sum.error);
        printf("%s\n", sqlite3_errstr(nr.sum.error));
        return 1;
    }
    printf("\nPassed\n");
    char * iter_prefix = "test/keys/key1";
    printf("Testing prefix key iterator\n");
    nr = vault_iter(&v, slice_ptr(iter_prefix, strlen(iter_prefix), strlen(iter_prefix)), print_item, NULL);
    if (IS_ERROR(nr)) {
        printf("\tFailed: returned an error code [%i]\n", nr.sum.error);
        printf("%s\n", sqlite3_errstr(nr.sum.error));
        return 1;
    }
    printf("\nPassed\n");

    printf("Testing deleting keys\n");
    for (size_t i = 0; i < countof(test_items); i++) {
        slice item_key = slice_ptr(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        nil_result_t n_result = vault_del(&v, item_key);
        if (IS_ERROR(n_result)) {
            printf("\tFailed: returned an error code [%i]\n", n_result.sum.error);
            printf("%s\n", sqlite3_errstr(n_result.sum.error));
            return 1;
        }
    }
    printf("\tPassed\n");

    printf("Testing getting deleted keys\n");
    for (size_t i = 0; i < countof(test_items); i++) {
        slice item_key = slice_ptr(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        guarded_result_t s_result = vault_get(&v, item_key);
        if (IS_ERROR(s_result)) {
            printf("\tFailed: returned an error code [%i]\n", s_result.sum.error);
            printf("%s\n", sqlite3_errstr(s_result.sum.error));
            return 1;
        }
        if (!IS_NULL(s_result)) {
            printf("\tFailed: found a deleted item\n");
            return 1;
        }
    }
    printf("\tPassed\n");

    destroy_vault(&v);
}