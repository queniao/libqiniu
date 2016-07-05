#ifndef __QN_STRING_H__
#define __QN_STRING_H__

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "base/basic_types.h"

#define posix_strlen strlen
#define posix_strcmp strcmp

#ifdef __cplusplus
extern "C"
{
#endif

typedef char * qn_string;

#define QN_STR_ARG_END (NULL)

#define qn_str_cstr(s) (s)

static inline int qn_str_size(const qn_string s)
{
    return posix_strlen(s);
}

static inline int qn_str_compare(const qn_string restrict s1, const qn_string restrict s2)
{
    return posix_strcmp(s1, s2);
}

extern qn_string qn_str_duplicate(const char * s);
extern qn_string qn_str_clone(const char * s, int sz);

static inline void qn_str_destroy(const char * s)
{
    free((void *)s);
}

extern qn_string qn_str_join_list(const char * restrict delimiter, const qn_string strs[], int n);
extern qn_string qn_str_join_va(const char * restrict delimiter, const qn_string restrict str1, const qn_string restrict str2, va_list ap);

static inline qn_string qn_str_join(const char * restrict deli, const qn_string restrict s1, const qn_string restrict s2, ...)
{
    va_list ap;
    qn_string new_str = NULL;
    va_start(ap, s2);
    new_str = qn_str_join_va(deli, s1, s2, ap);
    va_end(ap);
    return new_str;
} // qn_str_join

static inline qn_string qn_str_join_2(const char * d, const qn_string restrict s1, const qn_string restrict s2)
{
    return qn_str_join(d, s1, s2, NULL);
}

static inline qn_string qn_str_join_3(const qn_string d, const qn_string restrict s1, const qn_string restrict s2, const qn_string restrict s3)
{
    return qn_str_join(d, s1, s2, s3, NULL);
}

static inline qn_string qn_str_concat_list(const qn_string strs[], int n)
{
    return qn_str_join_list("", strs, n);
}

static inline qn_string qn_str_concat_va(const qn_string restrict s1, const qn_string restrict s2, va_list ap)
{
    return qn_str_join_va("", s1, s2, ap);
}

static inline qn_string qn_str_concat(const qn_string restrict s1, const qn_string restrict s2, ...)
{
    qn_string new_str = NULL;
    va_list ap;
    va_start(ap, s2);
    new_str = qn_str_join_va("", s1, s2, ap);
    va_end(ap);
    return new_str;
}

static inline qn_string qn_str_concat_2(const qn_string s1, const qn_string s2)
{
    return qn_str_join("", s1, s2, NULL);
}

static inline qn_string qn_str_concat_3(const qn_string s1, const qn_string s2, const qn_string s3)
{
    return qn_str_join("", s1, s2, s3, NULL);
}

extern qn_string qn_str_vprintf(const char * restrict format, va_list ap);
extern qn_string qn_str_sprintf(const char * restrict format, ...);
extern int qn_str_snprintf(char * restrict str, int size,  const char * restrict format, ...);

extern qn_string qn_str_encode_base64_urlsafe(const char * restrict bin, int bin_size);
extern qn_string qn_str_decode_base64_urlsafe(const char * restrict str, int str_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_STRING_H__
