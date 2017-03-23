#ifndef __QN_STRING_H__
#define __QN_STRING_H__

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#define posix_strlen strlen
#define posix_strcmp strcmp
#define posix_strncmp strncmp
#define posix_strchr strchr
#define posix_strstr strstr

#ifdef __cplusplus
extern "C"
{
#endif

typedef char * qn_string;

// ---- Declaration of C String ----

QN_SDK extern qn_string qn_cs_duplicate(const char * restrict s);
QN_SDK extern qn_string qn_cs_clone(const char * restrict s, qn_size sz);

QN_SDK extern qn_string qn_cs_join_list(const char * restrict delimiter, const char ** restrict ss, int n);
QN_SDK extern qn_string qn_cs_join_va(const char * restrict delimiter, const char * restrict s1, const char * restrict s2, va_list ap);

static inline qn_string qn_cs_concat(const char * restrict s1, const char * restrict s2, ...)
{
    qn_string new_str = NULL;
    va_list ap;
    va_start(ap, s2);
    new_str = qn_cs_join_va("", s1, s2, ap);
    va_end(ap);
    return new_str;
}

QN_SDK extern qn_string qn_cs_join_raw_va(const char * restrict delimiter, const char * restrict s1, qn_size s1_size, const char * restrict s2, qn_size s2_size, va_list ap);

static inline qn_string qn_cs_concat_raw(const char * restrict s1, qn_size s1_size, const char * restrict s2, qn_size s2_size, ...)
{
    qn_string new_str = NULL;
    va_list ap;
    va_start(ap, s2_size);
    new_str = qn_cs_join_raw_va("", s1, s1_size, s2, s2_size, ap);
    va_end(ap);
    return new_str;
}

QN_SDK extern qn_string qn_cs_vprintf(const char * restrict format, va_list ap);
QN_SDK extern qn_string qn_cs_sprintf(const char * restrict format, ...);
QN_SDK extern int qn_cs_snprintf(char * restrict buf, qn_size buf_size, const char * restrict format, ...);

QN_SDK extern qn_string qn_cs_encode_base64_urlsafe(const char * restrict bin, qn_size bin_size);
QN_SDK extern qn_string qn_cs_decode_base64_urlsafe(const char * restrict str, qn_size str_size);

typedef qn_bool (*qn_cs_percent_encode_check_fn)(int c);

QN_SDK extern qn_bool qn_cs_percent_encode_check(int c);
QN_SDK extern qn_ssize qn_cs_percent_encode_in_buffer_with_checker(char * restrict buf, qn_size buf_size, const char * restrict bin, qn_size bin_size, qn_cs_percent_encode_check_fn need_to_encode);
QN_SDK extern qn_string qn_cs_percent_encode_with_checker(const char * restrict bin, qn_size bin_size, qn_cs_percent_encode_check_fn need_to_encode);

#define qn_cs_percent_encode_in_buffer(buf, buf_size, bin, bin_size) qn_cs_percent_encode_in_buffer_with_checker(buf, buf_size, bin, bin_size, NULL)
#define qn_cs_percent_encode(bin, bin_size) qn_cs_percent_encode_with_checker(bin, bin_size, NULL)

// ---- Declaration of String ----

QN_SDK extern const qn_string qn_str_empty_string;

#define QN_STR_ARG_END (NULL)

#define qn_str_cstr(s) (s)

static inline qn_size qn_str_size(const qn_string restrict s)
{
    if (!s) return 0;
    return posix_strlen(s);
}

static inline int qn_str_compare(const qn_string restrict s1, const qn_string restrict s2)
{
    return posix_strcmp(s1, s2);
}

static inline const char * qn_str_find_char(const char * restrict s, int c)
{
    return (const qn_string)posix_strchr(s, c);
}

static inline const char * qn_str_find_char_or_null(const char * restrict s, int c)
{
    const char * pos = posix_strchr(s, c);
    return (const qn_string) ((pos) ? pos : (s + posix_strlen(s)));
}

static inline const qn_string qn_str_find_substring(const char * restrict s, const char * restrict sub)
{
    return (const qn_string)posix_strstr(s, sub);
}

static inline int qn_str_compare_raw(const qn_string restrict s1, const char * restrict s2)
{
    return posix_strcmp(s1, s2);
}

static inline qn_string qn_str_duplicate(const qn_string restrict s)
{
    return qn_cs_clone(qn_str_cstr(s), qn_str_size(s));
}

static inline void qn_str_destroy(const char * restrict s)
{
    if (s != qn_str_empty_string) {
        free((void *)s);
    } // if
}

QN_SDK extern qn_string qn_str_join_list(const char * restrict delimiter, const qn_string * restrict ss, int n);
QN_SDK extern qn_string qn_str_join_va(const char * restrict delimiter, const qn_string restrict s1, const qn_string restrict s2, va_list ap);

static inline qn_string qn_str_join(const char * restrict deli, const qn_string restrict s1, const qn_string restrict s2, ...)
{
    va_list ap;
    qn_string new_str;
    va_start(ap, s2);
    new_str = qn_str_join_va(deli, s1, s2, ap);
    va_end(ap);
    return new_str;
} // qn_str_join

static inline qn_string qn_str_join_2(const char * restrict d, const qn_string restrict s1, const qn_string restrict s2)
{
    return qn_str_join(d, s1, s2, NULL);
}

static inline qn_string qn_str_join_3(const qn_string restrict d, const qn_string restrict s1, const qn_string restrict s2, const qn_string restrict s3)
{
    return qn_str_join(d, s1, s2, s3, NULL);
}

static inline qn_string qn_str_concat_list(const qn_string * restrict strs, int n)
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

static inline qn_string qn_str_concat_2(const qn_string restrict s1, const qn_string restrict s2)
{
    return qn_str_join("", s1, s2, NULL);
}

static inline qn_string qn_str_concat_3(const qn_string restrict s1, const qn_string restrict s2, const qn_string restrict s3)
{
    return qn_str_join("", s1, s2, s3, NULL);
}

#ifdef __cplusplus
}
#endif

#endif // __QN_STRING_H__
