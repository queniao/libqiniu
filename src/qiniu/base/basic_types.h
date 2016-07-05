#ifndef __QN_BASIC_TYPES_H__
#define __QN_BASIC_TYPES_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t qn_uint32;
typedef uint64_t qn_uint64;
typedef size_t qn_size;

typedef bool qn_bool;

static const qn_bool qn_false = false;
static const qn_bool qn_true = true;

#ifndef QN_CFG_BIG_NUMBERS

typedef long qn_integer;
typedef double qn_number;

#else

typedef long long qn_integer;
typedef long double qn_number;

#endif

#endif // __QN_BASIC_TYPES_H__

