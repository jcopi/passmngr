#ifndef PASSMNGR_HASHMAP_H_
#define PASSMNGR_HASHMAP_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <common.h>
#include <result.h>

typedef struct map_node {
    byte_t*   key;
    size_t    key_size;
    uintptr_t value;
    uint8_t   dist;
    bool      used;
} map_node_t;

typedef struct map_probe_result {
    bool    found;
    bool    limit_exceeded;
    bool    smaller_found;
    bool    empty_found;
    uint8_t probe_len;
    size_t  index;
} map_probe_result_t;

typedef struct map {
    map_node_t* table;
    size_t      count;
    size_t      capacity;
    uint8_t     max_probe;

    uint32_t    hash_seed;

    void (*deallocator)(void*);
} map_t;

typedef enum map_error {
    MAP_KEY_NOT_FOUND,
    MAP_OUT_OF_MEMORY
} map_error_t;

RESULT_TYPE(hm_value_result_t, uintptr_t, map_error_t)
RESULT_EMPTY_TYPE(hm_empty_result_t, map_error_t)

#define MAP_INIT_CAPACITY (16)
#define MAP_INIT_MAX_PROBE (4)

hm_empty_result_t hm_init     (map_t* hm);
void              hm_destroy  (map_t* hm);

hm_empty_result_t hm_set      (map_t* hm, const byte_t* key_bytes, size_t key_size, uintptr_t value);
hm_empty_result_t hm_set_copy (map_t* hm, const byte_t* key_bytes, size_t key_size, uintptr_t value);
hm_empty_result_t hm_delete   (map_t* hm, const byte_t* key_bytes, size_t key_size, uintptr_t value);
hm_value_result_t hm_get      (map_t* hm, const byte_t* key_bytes, size_t key_size);

#endif