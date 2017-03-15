#ifndef __QN_OS_LINUX_TYPES_H__
#define __QN_OS_LINUX_TYPES_H__

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef unsigned int qn_uint;
typedef uint32_t qn_uint32;
typedef uint64_t qn_uint64;
typedef ssize_t qn_ssize;
typedef size_t qn_size;

typedef bool qn_bool;

static const qn_bool qn_false = false;
static const qn_bool qn_true = true;

#if defined(QN_CFG_BIG_NUMBERS)

typedef long long qn_integer;
typedef long double qn_number;

#else

typedef long qn_integer;
typedef double qn_number;

#endif

#if defined(QN_CFG_LARGE_FILE_SUPPORT)

typedef long long qn_fsize; // A signed long long integer for file sizes.
typedef off64_t qn_foffset; // A signed long long integer for file offsets.

#else

typedef long qn_fsize;      // A signed long integer for file sizes.
typedef off_t qn_foffset;   // A signed long integer for file offsets.

#endif

#endif // __QN_OS_LINUX_TYPES_H__

