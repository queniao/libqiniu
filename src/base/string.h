#ifndef __QN_STRING_H__
#define __QN_STRING_H__

#include "base/basic_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_STRING
{
    const char * str;
    qn_size size;
    char data[0];
} qn_string;

#define QN_STR_MAX_SIZE ((~ (qn_size)0L) - sizeof(qn_string) - 1)

extern qn_string * qn_str_create(const char * str, qn_size size);
extern qn_string * qn_str_duplicate(qn_string * src);
extern void qn_str_destroy(qn_string * self);

//== Function qn_str_join_strings()
//== Parameters:
//==    delimiter   A string as delimiter, can be en empty one("").
//==    ...         A list of `const char * str` and `qn_size size` arguments,
//==                the two must appear in pairs. End the list with a NULL value.
//== Return:
//==    A new string object.
//== ERRNO:
//==    ENOMEM      No enough memory for allocating buffer space.
//==    EINVALID    No pairs of string arguments are passed.
//==    EOVERFLOW   The size of the joined string would overflow the max string size.
extern qn_string * qn_str_join_strings(const char * delimiter, ...);

//extern qn_string * qn_str_sprintf(const char * restrict format, ...);
//extern qn_string * qn_str_snprintf(char * restrict str, qn_size size,  const char * restrict format, ...);

extern qn_bool qn_str_compare(const qn_string * restrict s1, const qn_string * restrict s2);

static inline qn_size qn_str_size(const qn_string * self)
{
    return self->size;
} // qn_str_size

static inline const char * qn_str_string(const qn_string * self)
{
    return self->str;
} // qn_str_string

#ifdef __cplusplus
}
#endif

#endif // __QN_STRING_H__
