#ifndef PASSMNGR_COMMON_H_
#define PASSMNGR_COMMON_H_

#include <stdint.h>
#include <assert.h>

typedef uint8_t byte_t;
static_assert(sizeof (byte_t) == 1, "Invalid byte size");

#define countof(A) (sizeof (A) / sizeof (A[0]))

#endif
