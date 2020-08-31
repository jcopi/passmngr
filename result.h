#ifndef __RESULT_H__
#define __RESULT_H__

#include <stdlib.h>

typedef unsigned char nil_t;
#define nil (0)

#define result_t_init(T1, T2) struct result_##T1##_##T2##_t { bool error; union {T1 value; T2 error;} value; };
#define result_t(T1, T2) struct result_##T1##_##T2##_t; 
#define result_ok(P, V) (p->error = false, p->value.value = (V), p)
#define result_err(P, E) (p->error = true, p->value.error = (E), p)
#define result_is_ok(R)  (!R.error)
#define result_is_err(R) (R.error)
#define result_unwrap(R) (R.error && exit(EXIT_FAILUTE), R.value.value)


#endif