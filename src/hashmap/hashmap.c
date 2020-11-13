#include <hashmap.h>

#include <stdlib.h>
#include <string.h>

#include <fasthash.h>

map_probe_result_t hm_int_probe         (map_t* hm, uint32_t hash, byte_t* key, size_t key_size);
hm_empty_result_t  hm_int_insert_copy   (map_t* hm, uint32_t hash, byte_t* key, size_t key_size, uintptr_t value);
hm_empty_result_t  hm_int_insert_nocopy (map_t* hm, uint32_t hash, byte_t* key, size_t key_size, uintptr_t value);
hm_empty_result_t  hm_int_remove        (map_t* hm, uint32_t hash, byte_t* key, size_t key_size);
hm_empty_result_t  hm_int_grow          (map_t* hm);
hm_empty_result_t  hm_int_shrink        (map_t* hm);
uint32_t           hm_int_hash          (map_t* hm, byte_t* key, size_t key_size);
bool               hm_int_keyeq         (byte_t* key1, byte_t* key2, size_t key1_bytes, size_t key2_bytes);

inline size_t hm_next_capacity (size_t capacity)
{
    return !capacity + (capacity * 2);
}

inline size_t hm_prev_capacity (size_t capacity)
{
    return capacity / 2;
}

inline size_t hm_shrink_threshold (size_t capacity)
{
    return (3 * hm_prev_capacity(capacity)) / 4;
}

inline size_t hm_mask(size_t x, size_t capacity)
{
    return x & (capacity - 1);
}

uint32_t hm_int_hash (map_t* hm, byte_t* key, size_t key_size) {
    return fasthash32(key, key_size, hm->hash_seed)
}

bool hm_int_keyeq (byte_t* key1, byte_t* key2, size_t key1_size, size_t key2_size)
{
    return (key1_size == key2_size) && memcpm(key1, key2, key1_size) == 0;
}

map_probe_result_t hm_int_probe (map_t* hm, uint32_t hash, byte_t* key, size_t key_size)
{
    map_probe_result_t result = {0};

    size_t index = hm_mask(hash, hm->capacity), i;
    uint8_t probe = 0;

    while (probe <= hm->max_probe) {
        i = hm_mask(index + probe, hm->capacity);

        if (!(hm->table[i].used)) {
            result.empty_found = true;
            result.probe_len = probe;
            result.index = i;

            return result;
        }

        if (hm->table[i].dist < probe) {
            result.smaller_found = true;
            result.probe_len = probe;
            result.index = i;

            return result;
        }

        if (hm->table[i].dist == probe && hm_int_keyeq(hm->table[i].key, key, hm->table[i].key_size, key_size)) {
            result.found = true;
            result.probe_len = probe;
            result.index = i;

            return result;
        }

        ++probe;
    }

    result.limit_exceeded = true;
    result.probe_len = probe;
    result.index = i;

    return result;
}

hm_empty_result_t hm_int_insert_nocopy (map_t* hm, uint32_t hash, byte_t* key, size_t key_size, uintptr_t value)
{
    hm_empty_result_t result = {0};
    map_probe_result_t probe;
    probe = hm_int_probe(hm, hash, key, key_size);

    if (probe.found) {
        if (NULL != hm->deallocator) {
            hm->deallocator(hm->table[probe.index].value);
        }
        hm->table[probe.index].value = value;

        return OK_EMPTY(result);
    }
    
    if (probe.empty_found) {
        hm->table[probe.index].used = true;
        hm->table[probe.index].dist = probe.probe_len;
        hm->table[probe.index].key = key;
        hm->table[probe.index].key_size = key_size;
        hm->table[probe.index].value = value;

        hm->count++;

        return OK_EMPTY(result);
    }

    if (probe.smaller_found) {
        byte_t* new_key = hm->table[probe.index].key;
        size_t new_key_size = hm->table[probe.index].key_size;
        uintptr_t new_value = hm->table[probe.index].value;
        uint32_t new_hash = hm_int_hash(hm, new_key, new_key_size);

        hm->table[probe.index].used = true;
        hm->table[probe.index].dist = probe.probe_len;
        hm->table[probe.index].key = key;
        hm->table[probe.index].key_size = key_size;
        hm->table[probe.index].value = value;

        // TODO: upon robinhood hashing the next probe could start at the end of the previous probe
        // to save a few iterations.
        return hm_int_insert_nocopy(hm, new_hash, new_key, new_key_size, new_value);
    }

    hm_empty_result_t maybe_grown = hm_int_grow(hm);
    if (IS_ERR(maybe_grown)) {
        return ERR(result, UNWRAP_ERR(maybe_grown));
    }

    return hm_int_insert_nocopy(hm, hash, key, key_size, value);
}

hm_empty_result_t hm_int_insert_copy (map_t* hm, uint32_t hash, byte_t* key, size_t key_size, uintptr_t value)
{
    hm_empty_result_t result = {0};
    map_probe_result_t probe;
    probe = hm_int_probe(hm, hash, key, key_size);

    if (probe.found) {
        if (NULL != hm->deallocator) {
            hm->deallocator(hm->table[probe.index].value);
        }
        hm->table[probe.index].value = value;

        return OK_EMPTY(result);
    }
    
    if (probe.empty_found) {
        byte_t* key_copy = calloc(key_size, 1);
        if (key_copy == NULL) {
            return ERR(result, MAP_OUT_OF_MEMORY);
        }
        memcpy(key_copy, key, key_size);

        hm->table[probe.index].used = true;
        hm->table[probe.index].dist = probe.probe_len;
        hm->table[probe.index].key = key_copy;
        hm->table[probe.index].key_size = key_size;
        hm->table[probe.index].value = value;

        hm->count++;

        return OK_EMPTY(result);
    }

    if (probe.smaller_found) {
        byte_t* key_copy = calloc(key_size, 1);
        if (key_copy == NULL) {
            return ERR(result, MAP_OUT_OF_MEMORY);
        }
        memcpy(key_copy, key, key_size);

        byte_t* new_key = hm->table[probe.index].key;
        size_t new_key_size = hm->table[probe.index].key_size;
        uintptr_t new_value = hm->table[probe.index].value;
        uint32_t new_hash = hm_int_hash(hm, new_key, new_key_size);

        hm->table[probe.index].used = true;
        hm->table[probe.index].dist = probe.probe_len;
        hm->table[probe.index].key = key_copy;
        hm->table[probe.index].key_size = key_size;
        hm->table[probe.index].value = value;

        // TODO: upon robinhood hashing the next probe could start at the end of the previous probe
        // to save a few iterations.
        return hm_int_insert_nocopy(hm, new_hash, new_key, new_key_size, new_value);
    }

    hm_empty_result_t maybe_grown = hm_int_grow(hm);
    if (IS_ERR(maybe_grown)) {
        return ERR(result, UNWRAP_ERR(maybe_grown));
    }

    return hm_int_insert_copy(hm, hash, key, key_size, value);
}

hm_empty_result_t hm_int_remove (map_t* hm, uint32_t hash, byte_t* key, size_t key_size)
{
    hm_empty_result_t result = {0};
    map_probe_result_t probe;
    probe = hm_int_probe(hm, hash, key, key_size);

    if (probe.found) {
       free(hm->table[probe.index].key);
       if (NULL != hm->deallocator) {
           hm->deallocator(hm->table[probe.index].value);
       }
       hm->table[probe.index] = (map_node_t){0};
       hm->count--;

       size_t index = probe.index, pindex = probe.index, probe_len = 0;
       for (;;) {
           probe_len++;
           index = hm_mask(probe.index + probe_len, hm->capacity);

           if (!hm->table[index].used || hm->table[index].dist == 0) {
               break;
           } else {
               hm->table[pindex] = hm->table[index];
               hm->table[pindex].dist--;
               hm->table[index] = (map_node_t){0};
           }

           pindex = index;
       }

       return hm_int_shrink(hm);
    }

    return OK_EMPTY(result);
}

hm_empty_result_t hm_int_grow (map_t* hm)
{
    hm_empty_result_t result = {0};
    
    size_t next_cap = hm_next_capacity(hm->capacity);
    uint8_t prev_max_probe = hm->max_probe;
    uint8_t next_max_probe = hm->max_probe + 1; //hm_int_log2(next_cap);

    map_node_t* prev_table = hm->table;
    size_t prev_count = hm->count;
    size_t prev_capacity = hm->capacity;

    hm->table = calloc(next_cap, sizeof (map_node_t));
    if (hm->table == NULL) {
        // Allocation failed put everything back as it was previously
        hm->table = prev_table;
        return ERR(result, MAP_OUT_OF_MEMORY);
    }

    hm->capacity = next_cap;
    hm->max_probe = next_max_probe;
    hm->count = 0;
    size_t i, j;
    for (i = 0, j = 0; i < prev_capacity && j < prev_count; ++i) {
        if (prev_table[i].used) {
            ++j;
            uint32_t hash = hm_int_hash(hm, prev_table[i].key, prev_table[i].key_size);
            hm_empty_result_t maybe_inserted = hm_int_insert_nocopy(hm, hash, prev_table[i].key, prev_table[i].key_size, prev_table[i].value);
            if (IS_ERR(maybe_inserted)) {
                // Failed to insert an item, put map back the way it was
                free(hm->table);
                hm->table = prev_table;
                hm->count = prev_count;
                hm->capacity = prev_capacity;
                hm->max_probe = prev_max_probe;
                
                return ERR(result, UNWRAP_ERR(maybe_inserted));
            }
        }
    }

    free(prev_table);
    return OK_EMPTY(result);

}

hm_empty_result_t hm_int_shrink (map_t* hm)
{
    hm_empty_result_t result = {0};

    if (hm->count >= hm_shrink_threshold(hm->capacity) || hm->capacity <= MAP_INIT_CAPACITY) {
        return OK_EMPTY(result);
    }

    size_t next_cap = hm_prev_capacity(hm->capacity);
    uint8_t prev_max_probe = hm->max_probe;
    uint8_t next_max_probe = hm->max_probe - 1; //hm_int_log2(next_cap);

    map_node_t* prev_table = hm->table;
    size_t prev_count = hm->count;
    size_t prev_capacity = hm->capacity;

    hm->table = calloc(next_cap, sizeof (map_node_t));
    if (hm->table == NULL) {
        // Allocation failed put everything back as it was previously
        hm->table = prev_table;
        return ERR(result, MAP_OUT_OF_MEMORY);
    }

    hm->capacity = next_cap;
    hm->max_probe = next_max_probe;
    hm->count = 0;
    size_t i, j;
    for (i = 0, j = 0; i < prev_capacity && j < prev_count; ++i) {
        if (prev_table[i].used) {
            ++j;
            uint32_t hash = hm_int_hash(hm, prev_table[i].key, prev_table[i].key_size);
            hm_empty_result_t maybe_inserted = hm_int_insert_nocopy(hm, hash, prev_table[i].key, prev_table[i].key_size, prev_table[i].value);
            if (IS_ERR(maybe_inserted)) {
                // Failed to insert an item, put map back the way it was
                free(hm->table);
                hm->table = prev_table;
                hm->count = prev_count;
                hm->capacity = prev_capacity;
                hm->max_probe = prev_max_probe;
                
                return ERR(result, UNWRAP_ERR(maybe_inserted));
            }
        }
    }

    free(prev_table);
    return OK_EMPTY(result);
}

hm_empty_result_t hm_init (map_t* hm)
{
    hm_empty_result_t result = {0};
    
    *hm = (map_t){0};
    hm->capacity = MAP_INIT_CAPACITY;
    hm->max_probe = MAP_INIT_MAX_PROBE;
    srand(time(NULL));
    hm->hash_seed = (uint32_t)rand;

    hm->table = calloc(hm->capacity, sizeof (map_node_t));
    if (NULL == hm->table) {
        return ERR(result, MAP_OUT_OF_MEMORY);
    }

    return OK_EMPTY(result);
}

void hm_destroy (map_t* hm) {
    size_t i;
    for (i = 0; i < hm->capacity; i++) {
        if (hm->table[i].used) {
            free(hm->table[i].key);
            if (NULL != hm->deallocator) {
                hm->deallocator(hm->table[i].value);
            }
        }
    }

    free(hm->table);
    *hm = (map_t){0};

    return;
}

hm_empty_result_t hm_set (map_t* hm, void* key, size_t key_size, void* value)
{
    return hm_int_insert_nocopy(hm, hm_int_hash(hm, key, key_size), key, key_size, value);
}

hm_empty_result_t hm_set_copy (map_t* hm, void* key, size_t key_size, void* value)
{
    return hm_int_insert_copy(hm, hm_int_hash(hm, key, key_size), key, key_size, value);
}

hm_empty_result_t hm_delete (map_t* hm, void* key, size_t key_size)
{
    return hm_int_remove(hm, hm_int_hash(hm, key, key_size), key, key_size);
}

hm_value_result_t hm_get (map_t* hm, void* key, size_t key_size)
{
    hm_value_result_t result = {0};

    map_probe_result_t probe;
    probe = hm_int_probe(hm, hm_int_hash(hm, key, key_size), key, key_size);
    if (probe.found) {
        return OK(result, hm->table[probe.index].value);
    }

    return ERR(result, MAP_KEY_NOT_FOUND);
}
