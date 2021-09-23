#pragma once

#include <sodium.h>
#include <assert.h>

#define KV_BLOCK_SIZE (4096)
#define KV_CIPHERTEXT_BLOCK_SIZE (KV_BLOCK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES)
#define KV_KEY_SIZE   (crypto_secretstream_xchacha20poly1305_KEYBYTES)

#define ITEM_KEY_LENGTH_TYPE     uint16_t
#define ITEM_VALUE_LENGTH_TYPE   uint16_t
#define ITEM_TIMESTAMP_TYPE      uint64_t

#define SIZEOF_ITEM_HEADER (sizeof (ITEM_KEY_LENGTH_TYPE) + sizeof (ITEM_VALUE_LENGTH_TYPE) + sizeof (ITEM_TIMESTAMP_TYPE))
#define MIN_ITEM_SIZE      (SIZEOF_ITEM_HEADER + (1) + (1))
#define MAX_ITEM_KEY_SIZE  (KV_BLOCK_SIZE - (SIZEOF_ITEM_HEADER + (1)))
#define MAX_ITEM_VALUE_SIZE  (KV_BLOCK_SIZE - (SIZEOF_ITEM_HEADER + (1)))

static_assert((ITEM_KEY_LENGTH_TYPE)KV_BLOCK_SIZE == KV_BLOCK_SIZE, "Invalid item key length type");
static_assert((ITEM_VALUE_LENGTH_TYPE)KV_BLOCK_SIZE == KV_BLOCK_SIZE, "Invalid item value length type");

#define MAX_PATH_LENGTH          (4096)
#define DEFAULT_UNLOCK_DURATION  (300000) // 5min in ms