#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <kv_store.h>

kv_item_t test_items[] = {
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

#define MASTER_PASSWORD        ("fillip-barratry-skeptic-niggle-wardrobe-feed-pastime-paranoia")
#define MASTER_PASSWORD_LENGTH (strlen(MASTER_PASSWORD))
#define KV_READ_FILENAME       ("./vault.hex")
#define KV_WRITE_FILENAME      ("./~vault.hex")

int main ()
{
    kv_t kv = {0};
    kv_empty_result_t maybe_empty = {0};

    printf("Testing Initialization: Expecting no errors\n");
    maybe_empty = kv_init(&kv, KV_READ_FILENAME, KV_WRITE_FILENAME);
    if (IS_ERR(maybe_empty)) {
        printf("  - FAILED with error code: %02i\n\n", UNWRAP_ERR(maybe_empty));
        return -1;
    }
    printf("  - PASSED\n\n");

    if (access(KV_READ_FILENAME, F_OK) == -1) {
        remove(KV_READ_FILENAME);
    }

    printf("Testing KV Creation: Expecting no errors\n");
    if (access(KV_READ_FILENAME, F_OK) == -1) {
        maybe_empty = kv_create(&kv, MASTER_PASSWORD, MASTER_PASSWORD_LENGTH);
        if (IS_ERR(maybe_empty)) {
            printf("  - FAILED with error code: %02i\n\n", UNWRAP_ERR(maybe_empty));
            return -1;
        }
    }
    printf("  - PASSED\n\n");

    printf("Testing KV Open/Unlock: Expecting no errors\n");
    maybe_empty = kv_open(&kv, MASTER_PASSWORD, MASTER_PASSWORD_LENGTH);
    if (IS_ERR(maybe_empty)) {
        printf("  - FAILED with error code: %02i\n\n", UNWRAP_ERR(maybe_empty));
        return -1;
    }
    printf("  - PASSED\n\n");

    printf("Testing Setting Keys: Expecting no errors\n");
    for (size_t i = 0; i < sizeof (test_items); i++) {
        maybe_empty = kv_set(&kv, test_items[i]);
        if (IS_ERR(maybe_empty)) {
            printf("  - FAILED with error code: %02i\n  - Failed while setting key '%s' to value '%s'\n\n", UNWRAP_ERR(maybe_empty), test_items[i].key.data, test_items[i].value.data);
            return -1;
        }
    }
    printf("  - PASSED\n\n");

    printf("Testing Getting Keys: Expecting values match sets and no errors\n");
    for (size_t i = 0; i < sizeof (test_items); i++) {
        kv_value_result_t maybe_value = kv_get(&kv, test_items[i].key);
        if (IS_ERR(maybe_value)) {
            printf("  - FAILED with error code: %02i\n  - Failed while setting key '%s' to value '%s'\n\n", UNWRAP_ERR(maybe_empty), test_items[i].key.data, test_items[i].value.data);
            return -1;
        }

        kv_value_t val = UNWRAP(maybe_value);
        if (memcmp(val.data, test_items[i].value.data, val.length) != 0) {
            printf("  - FAILED unexpected value returned\n  - For key '%s' expected value '%s' got '%s'\n\n", test_items[i].key.data, test_items[i].value.data, val.data);
            return -1;
        }
    }
    printf("  - PASSED\n\n");

    printf("Testing KV Close/Lock: Expecting no errors\n");
    maybe_empty = kv_close(&kv);
    if (IS_ERR(maybe_empty)) {
        printf("  - FAILED with error code: %02i\n\n", UNWRAP_ERR(maybe_empty));
        return -1;
    }
    printf("  - PASSED\n\n");

    return 0;
}