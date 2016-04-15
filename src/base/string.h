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

static inline qn_size qn_str_size(const qn_string * self)
{
    return self->size;
} // qn_str_size

static inline const char * qn_str_string(const qn_string * self)
{
    return self->str;
} // qn_str_string

extern qn_string * qn_str_create(const char * str, qn_size size);
extern qn_string * qn_str_duplicate(qn_string * src);
extern void qn_str_destroy(qn_string * self);

//== Function qn_str_join_raw()
//== Parameters:
//==    delimiter   A string passed as the delimiter, can be en empty one("").
//==                Passing a NULL pointer will return EINVALID.
//==    s1          A pointer to the first raw string to join.
//==    s1_size     The size of the first raw string.
//==    s2          A pointer to the second raw string to join.
//==    s2_size     The size of the second raw string.
//==    ...         A list of `const char * str` and `qn_size size` arguments,
//==                the two must appear in pairs. End the list with a NULL value.
//== Return:
//==    A new string object or a NULL pointer.
//== ERRNO:
//==    ENOMEM      No enough memory for allocating buffer space.
//==    EINVALID    The delimiter/s1/s2 is NULL.
//==    EOVERFLOW   The size of the joined string would overflow the max string size(QN_STR_MAX_SIZE).
extern qn_string * qn_str_join_raw(const char * restrict delimiter, const char * restrict s1, qn_size s1_size, const char * restrict s2, qn_size s2_size, ...);

#define qn_str_join_raw_2(deli, s1, s1_size, s2, s2_size) qn_str_join_raw(deli, s1, s1_size, s2, s2_size, NULL)
#define qn_str_join_raw_3(deli, s1, s1_size, s2, s2_size, s3, s3_size) qn_str_join_raw(deli, s1, s1_size, s2, s2_size, s3, s3_size, NULL)
#define qn_str_concat_raw_2(s1, s1_size, s2, s2_size) qn_str_join_raw("", s1, s1_size, s2, s2_size, NULL)
#define qn_str_concat_raw_3(s1, s1_size, s2, s2_size, s3, s3_size) qn_str_join_raw("", s1, s1_size, s2, s2_size, s3, s3_size, NULL)
#define qn_str_concat_raw(s1, s1_size, s2, s2_size, ...) qn_str_join_raw("", s1, s1_size, s2, s2_size, __VA_ARGS__)

//== Function qn_str_join()
//== Parameters:
//==    delimiter   A string passed as the delimiter, can be en empty one("").
//==                Passing a NULL pointer will return EINVALID.
//==    strs        A pointer to the array of string objects to join.
//==    n           An int argument indicating the number of joining string objects.
//== Return:
//==    A new string object or a NULL pointer.
//== ERRNO:
//==    ENOMEM      No enough memory for allocating buffer space.
//==    EINVALID    The delimiter is NULL, or no string objects is passed.
//==    EOVERFLOW   The size of the joined string would overflow the max string size(QN_STR_MAX_SIZE).
extern qn_string * qn_str_join(const char * restrict delimiter, qn_string strs[], int n);

#define qn_str_concat(strs, n) qn_str_join("", strs, n)

//extern qn_string * qn_str_sprintf(const char * restrict format, ...);
//extern qn_string * qn_str_snprintf(char * restrict str, qn_size size,  const char * restrict format, ...);

extern qn_bool qn_str_compare(const qn_string * restrict s1, const qn_string * restrict s2);

#ifdef __cplusplus
}
#endif

#endif // __QN_STRING_H__
