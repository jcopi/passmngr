#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <kv_store.h>
#include <wordlist.h>

#define MASTER_PASSWORD        ("fillip-barratry-skeptic-niggle-wardrobe-feed-pastime-paranoia")
#define MASTER_PASSWORD_LENGTH (strlen(MASTER_PASSWORD))
#define KV_READ_FILENAME       ("./vault.hex")
#define KV_WRITE_FILENAME      ("./~vault.hex")

#ifdef WIN32

#include <windows.h>
double get_time()
{
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double)t.QuadPart/(double)f.QuadPart;
}

#else

#include <sys/time.h>
#include <sys/resource.h>

double get_time()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec*1e-6;
}

#endif


int main ()
{
    kv_t kv = {0};
    kv_empty_result_t maybe_empty = {0};
    double start, end;

    start = get_time();
    printf("Benchmarking KV Initialize Performance\n");
    maybe_empty = kv_init(&kv, KV_READ_FILENAME, KV_WRITE_FILENAME);
    if (IS_ERR(maybe_empty)) {
        printf("  - FAILED to initialize KV with error code: %02i\n\n", UNWRAP_ERR(maybe_empty));
        return -1;
    }
    end = get_time();
    printf("  - KV Initialization took %lf seconds\n\n", end - start);

    if (access(KV_READ_FILENAME, F_OK) == -1) {
        remove(KV_READ_FILENAME);
    }

    printf("Benchmarking KV Creation Performance\n");
    if (access(KV_READ_FILENAME, F_OK) == -1) {
        start = get_time();
        maybe_empty = kv_create(&kv, MASTER_PASSWORD, MASTER_PASSWORD_LENGTH);
        if (IS_ERR(maybe_empty)) {
            printf("  - FAILED to create KV with error code: %02i\n\n", UNWRAP_ERR(maybe_empty));
            return -1;
        }
        end = get_time();
    }
    printf("  - KV Creation took %f seconds\n\n", end - start);

    printf("Benchmarking KV Open/Unlock Performance\n");
    start = get_time();
    maybe_empty = kv_open(&kv, MASTER_PASSWORD, MASTER_PASSWORD_LENGTH);
    if (IS_ERR(maybe_empty)) {
        printf("  - FAILED with error code: %02i\n\n", UNWRAP_ERR(maybe_empty));
        return -1;
    }
    end = get_time();
    printf("  - KV Unlocking took %f seconds\n\n", end - start);


    kv_item_t* items = calloc(WORDLIST_LENGTH, sizeof(kv_item_t));
    for (size_t i = 0; i < WORDLIST_LENGTH; i++) {
        items[i].key.length = strlen(WORDLIST[i]);
        memcpy(items[i].key.data, WORDLIST[i], items[i].key.length);
        items[i].value.length = strlen(WORDLIST[i]);
        memcpy(items[i].value.data, WORDLIST[i], items[i].value.length);
    }

    printf("Benchmarking Setting Keys\n");
    size_t i;
    start = get_time();
    for (i = 0; i < WORDLIST_LENGTH; i++) {
        maybe_empty = kv_set(&kv, items[i]);
        if (IS_ERR(maybe_empty)) {
            printf("  - FAILED with error code: %02i\n  - Failed while setting key '%s' to value '%s'\n\n", UNWRAP_ERR(maybe_empty), items[i].key.data, items[i].value.data);
            return -1;
        }
    }
    end = get_time();
    printf("  - KV Set %li keys in %f seconds\n", i, end - start);
    printf("  - %lf seconds per key\n", (end - start) / (double)i);
    printf("  - %lf keys per second\n\n", i / (end - start));

    printf("Testing Getting Keys: Expecting values match sets and no errors\n");
    start = get_time();
    for (i = 0; i < WORDLIST_LENGTH; i++) {
        kv_value_result_t maybe_value = kv_get(&kv, items[i].key);
        if (IS_ERR(maybe_value)) {
            printf("  - FAILED with error code: %02i\n  - Failed while getting key '%s'\n\n", UNWRAP_ERR(maybe_empty), items[i].key.data);
            return -1;
        }

        kv_value_t val = UNWRAP(maybe_value);
        if (memcmp(val.data, items[i].value.data, val.length) != 0) {
            printf("  - FAILED unexpected value returned\n  - For key '%s' expected value '%s' got '%s'\n\n", items[i].key.data, items[i].value.data, val.data);
            return -1;
        }
    }
    end = get_time();
    printf("  - KV Got %li values in %f seconds\n\n", i, end - start);
    printf("  - %lf seconds per key\n", (end - start) / (double)i);
    printf("  - %lf keys per second\n\n", i / (end - start));

    printf("Benchmarking KV Close/Lock\n");
    start = get_time();
    maybe_empty = kv_close(&kv);
    if (IS_ERR(maybe_empty)) {
        printf("  - FAILED with error code: %02i\n\n", UNWRAP_ERR(maybe_empty));
        return -1;
    }
    end = get_time();
    printf("  - KV Locking took %f seconds\n\n", end - start);

    return 0;
}