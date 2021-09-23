#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

#define MASTER_PASSWORD        ("fillip-barratry-skeptic-niggle-wardrobe-feed-pastime-paranoia")
#define MASTER_PASSWORD_2      ("feed-fillip-paranoia-skeptic-barratry-wardrobe-pastime-niggle")
#define MASTER_PASSWORD_LENGTH (strlen(MASTER_PASSWORD))
#define MASTER_KEY             {0xDE,0xAD,0xBE,0xEF,0x00,0x00,0x00,0x00,0xDE,0xAD,0xBE,0xEF,0x00,0x00,0x00,0x00,0xDE,0xAD,0xBE,0xEF,0x00,0x00,0x00,0x00,0xDE,0xAD,0xBE,0xEF,0x00,0x00,0x00,0x00}
#define KV_READ_FILENAME       ("./vault.hex")
#define KV_WRITE_FILENAME      ("./~vault.hex")

bool valid;

int main ()
{
    kv_t kv = {0};
    char* read_file = KV_READ_FILENAME;
    char* write_file = KV_WRITE_FILENAME;
    uint8_t key[KV_KEY_SIZE] = MASTER_KEY;

    printf("Testing Initialization: Expecting no errors\n");
    kv_error_t err = kv_init(&kv, s_create(read_file, strlen(read_file), strlen(read_file)), s_create(write_file, strlen(write_file), strlen(write_file)));
    if (err.is_error) {
        printf("  - FAILED with error code: %02i\n\n", err.code);
        return -1;
    }
    printf("  - PASSED\n\n");

    remove(read_file);

    slice_t key_buffer = kv_key_buffer(&kv);
    slice_t key_slice = s_create(key, sizeof (key), sizeof (key));
    s_append(key_buffer, key_slice, key_slice.len);

    printf("Testing KV Open/Unlock: Expecting no errors\n");
    err = kv_unlock(&kv);
    if (err.is_error) {
        printf("  - FAILED with error code: %02i\n\n", err.code);
        return -1;
    }
    printf("  - PASSED\n\n");

    printf("Testing KV Creation: Expecting no errors\n");
    err = kv_create(&kv); // MASTER_PASSWORD, MASTER_PASSWORD_LENGTH);
    if (err.is_error) {
        printf("  - FAILED with error code: %02i\n\n", err.code);
        return -1;
    }
    printf("  - PASSED\n\n");

    printf("Testing Setting Plain Keys: Expecting no errors\n");
    for (size_t i = 0; i < countof(test_items) && i < 5; i++) {
        slice_t item_key = s_create(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        slice_t item_value = s_create(test_items[i].value.data, test_items[i].value.length, test_items[i].value.length);
        err = kv_plain_set(&kv, item_key, item_value);
        if (err.is_error) {
            printf("  - FAILED with error code: %02i\n  - Failed while setting key '%s' to value '%s'\n\n", err.code, test_items[i].key.data, test_items[i].value.data);
            return -1;
        }
    }
    printf("  - PASSED\n\n");

    printf("Testing Setting Keys: Expecting no errors\n");
    for (size_t i = 0; i < countof(test_items); i++) {
        slice_t item_key = s_create(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        slice_t item_value = s_create(test_items[i].value.data, test_items[i].value.length, test_items[i].value.length);
        err = kv_set(&kv, item_key, item_value);
        if (err.is_error) {
            printf("  - FAILED with error code: %02i\n  - Failed while setting key '%s' to value '%s'\n\n", err.code, test_items[i].key.data, test_items[i].value.data);
            return -1;
        }
    }
    printf("  - PASSED\n\n");

    printf("Testing Getting Plain Keys: Expecting values match sets and no errors\n");
    for (size_t i = 0; i < countof(test_items) && i < 5; i++) {
        slice_t item_key = s_create(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        slice_t item_value = s_create(test_items[i].value.data, test_items[i].value.length, test_items[i].value.length);
        result_uint64_t maybe_u64 = kv_reserve_get(&kv);
        if (maybe_u64.is_error) {
            printf("  - FAILED with error code: %02i\n  - Failed while getting key '%s'\n\n", result_uint64_unwrap_err(maybe_u64).code, test_items[i].key.data);
            return -1;
        }
        uint64_t reservation = result_uint64_unwrap(maybe_u64);
        result_opt_item_t maybe_value = kv_plain_get(&kv, reservation, item_key, 0);
        if (maybe_value.is_error) {
            printf("  - FAILED with error code: %02i\n  - Failed while getting key '%s'\n\n", result_opt_item_unwrap_err(maybe_value).code, test_items[i].key.data);
            return -1;
        }
        optional_item_t opt_item = result_opt_item_unwrap(maybe_value);
        if (!opt_item.is_available) {
            printf("  - FAILED requested key not in kv store\n  - Failed while getting key '%s'\n\n", test_items[i].key.data);
            return -1;
        }
        kv_item_t item = optional_item_unwrap(opt_item);
        
        if (item.value.len != item_value.len || memcmp(item.value.ptr, item_value.ptr, item_value.len) != 0) {
            printf("  - FAILED unexpected value returned\n  - For key '%s' expected value '%s' got '%s'\n\n", test_items[i].key.data, test_items[i].value.data, item.value.ptr);
            return -1;
        }
        err = kv_release_get(&kv, reservation);
        if (err.is_error) {
            printf("  - FAILED could not return reservation\n  - Failed with error code %02i\n\n", err.code);
            return -1;
        }
    }
    printf("  - PASSED\n\n");

    printf("Testing Getting Keys: Expecting values match sets and no errors\n");
    for (size_t i = 0; i < countof(test_items); i++) {
        slice_t item_key = s_create(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        slice_t item_value = s_create(test_items[i].value.data, test_items[i].value.length, test_items[i].value.length);
        result_uint64_t maybe_u64 = kv_reserve_get(&kv);
        if (maybe_u64.is_error) {
            printf("  - FAILED with error code: %02i\n  - Failed while getting key '%s'\n\n", result_uint64_unwrap_err(maybe_u64).code, test_items[i].key.data);
            return -1;
        }
        uint64_t reservation = result_uint64_unwrap(maybe_u64);
        result_opt_item_t maybe_value = kv_get(&kv, reservation, item_key, 0);
        if (maybe_value.is_error) {
            printf("  - FAILED with error code: %02i\n  - Failed while getting key '%s'\n\n", result_opt_item_unwrap_err(maybe_value).code, test_items[i].key.data);
            return -1;
        }
        optional_item_t opt_item = result_opt_item_unwrap(maybe_value);
        if (!opt_item.is_available) {
            printf("  - FAILED requested key not in kv store\n  - Failed while getting key '%s'\n\n", test_items[i].key.data);
            return -1;
        }
        kv_item_t item = optional_item_unwrap(opt_item);
        
        if (item.value.len != item_value.len || memcmp(item.value.ptr, item_value.ptr, item_value.len) != 0) {
            printf("  - FAILED unexpected value returned\n  - For key '%s' expected value '%s' got '%s'\n\n", test_items[i].key.data, test_items[i].value.data, item.value.ptr);
            return -1;
        }
        err = kv_release_get(&kv, reservation);
        if (err.is_error) {
            printf("  - FAILED could not return reservation\n  - Failed with error code %02i\n\n", err.code);
            return -1;
        }
    }
    printf("  - PASSED\n\n");

    printf("Testing Deleting Keys: Expecting no errors\n");
    for (size_t i = 0; i < countof(test_items); i++) {
        slice_t item_key = s_create(test_items[i].key.data, test_items[i].key.length, test_items[i].key.length);
        err = kv_delete(&kv, item_key);
        if (err.is_error) {
            printf("  - FAILED with error code: %02i\n  - Failed while deleting key '%s'\n\n", err.code, test_items[i].key.data);
            return -1;
        }
    }
    printf("  - PASSED\n\n");

    
    printf("Testing KV Close/Lock: Expecting no errors\n");
    err = kv_lock(&kv);
    if (err.is_error) {
        printf("  - FAILED with error code: %02i\n\n", err.code);
        return -1;
    }
    printf("  - PASSED\n\n");

    return 0;
}