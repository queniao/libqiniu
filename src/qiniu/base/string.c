#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include "qiniu/base/string.h"
#include "qiniu/base/base64.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

qn_string qn_str_duplicate(const char * s)
{
    return qn_str_clone(s, strlen(s));
}

qn_string qn_str_clone(const char * s, int sz)
{
    qn_string new_str = malloc(sz + 1);
    if (new_str) {
        memcpy(new_str, s, sz);
        new_str[sz] = '\0';
    } else {
        qn_err_set_no_enough_memory();
    }
    return new_str;
}

qn_string qn_str_join_list(const char * restrict deli, const qn_string ss[], int n)
{
    qn_string new_str = NULL;
    char * pos = NULL;
    int final_size = 0;
    int deli_size = 0L;
    int i = 0;

    if (n == 1) return qn_str_duplicate(ss[0]);

    deli_size = strlen(deli);

    for (i = 0; i < n; i += 1) {
        final_size += qn_str_size(ss[i]);
    } // for
    final_size += deli_size * (n - 1);

    pos = new_str = malloc(final_size + 1);
    if (!new_str) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    memcpy(pos, qn_str_cstr(ss[0]), qn_str_size(ss[0]));
    pos += qn_str_size(ss[0]);
    for (i = 1; i < n; i += 1) {
        memcpy(pos, deli, deli_size);
        pos += deli_size;
        memcpy(pos, qn_str_cstr(ss[i]), qn_str_size(ss[i]));
        pos += qn_str_size(ss[i]);
    } // for

    new_str[final_size] = '\0';
    return new_str;
}

qn_string qn_str_join_va(const char * restrict deli, const qn_string restrict s1, const qn_string restrict s2, va_list ap)
{
    va_list cp;
    qn_string new_str = NULL;
    qn_string str = NULL;
    int final_size = 0;
    int deli_size = strlen(deli);
    int n = 0;
    char * pos = NULL;

    if (!s1) return NULL;
    if (!s2) return qn_str_duplicate(s1);

    final_size = qn_str_size(s1) + deli_size + qn_str_size(s2);

    va_copy(cp, ap);
    while ((str = va_arg(cp, qn_string))) {
        final_size += deli_size + qn_str_size(str);
        n += 1;
    } // while
    va_end(cp);

    pos = new_str = malloc(final_size + 1);
    if (!new_str) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    memcpy(pos, qn_str_cstr(s1), qn_str_size(s1));
    pos += qn_str_size(s1);
    memcpy(pos, deli, deli_size);
    pos += deli_size;
    memcpy(pos, qn_str_cstr(s2), qn_str_size(s2));
    pos += qn_str_size(s2);

    if (n > 0) {
        va_copy(cp, ap);
        while ((str = va_arg(cp, qn_string))) {
            memcpy(pos, deli, deli_size);
            pos += deli_size;
            memcpy(pos, qn_str_cstr(str), qn_str_size(str));
            pos += qn_str_size(str);
        } // while
        va_end(cp);
    } // if

    new_str[final_size] = '\0';
    return new_str; 
}

qn_string qn_str_vprintf(const char * restrict format, va_list ap)
{
    va_list cp;
    qn_string new_str = NULL;
    int printed_size = 0;

    va_copy(cp, ap);
#if defined(_MSC_VER)
    printed_size = vsnprintf(0x1, 1, format, cp);
#else
    printed_size = vsnprintf(NULL, 0, format, cp);
#endif
    va_end(cp);

    if (printed_size < 0) {
        // Keep the errno set by vsnprintf.
        return NULL;
    }
    if (printed_size == 0) {
        return "";
    }

    new_str = malloc(printed_size + 1);
    if (!new_str) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    va_copy(cp, ap);
    printed_size = vsnprintf(new_str, printed_size + 1, format, cp);
    va_end(cp);

    if (printed_size < 0) {
        // Keep the errno set by vsnprintf.
        qn_str_destroy(new_str);
        return NULL;
    }
    return new_str;
} // qn_str_vprintf

qn_string qn_str_sprintf(const char * restrict format, ...)
{
    va_list ap;
    qn_string new_str = NULL;

    va_start(ap, format);
    new_str = qn_str_vprintf(format, ap);
    va_end(ap);
    return new_str;
} // qn_str_printf

#if defined(_MSC_VER)

#if (_MSC_VER < 1400)
#error The version of the MSVC is lower then VC++ 2005.
#endif

int qn_str_snprintf(char * restrict str, int size,  const char * restrict format, ...)
{
    va_list ap;
    int printed_size = 0;
    char * buf = str;
    int buf_cap = size;

    if (str == NULL || size == 0) {
        buf = 0x1;
        buf_cap = 1;
    } // if

    va_start(ap, format);
    printed_size = vsnprintf(buf, buf_cap, format, ap);
    va_end(ap);
    return printed_size;
} // qn_str_snprintf

#else

int qn_str_snprintf(char * restrict str, int size,  const char * restrict format, ...)
{
    va_list ap;
    int printed_size = 0;

    va_start(ap, format);
    printed_size = vsnprintf(str, size, format, ap);
    va_end(ap);
    // TODO: Convert system errno to local errors.
    return printed_size;
} // qn_str_snprintf

#endif

qn_string qn_str_encode_base64_urlsafe(const char * restrict bin, int bin_size)
{
    qn_string new_str = NULL;
    int encoding_size = qn_b64_encode_urlsafe(NULL, 0, bin, bin_size, QN_B64_APPEND_PADDING);
    
    if (encoding_size == 0) {
        return "";
    }

    new_str = malloc(encoding_size + 1);
    if (!new_str) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    qn_b64_encode_urlsafe(new_str, encoding_size, bin, bin_size, QN_B64_APPEND_PADDING);
    new_str[encoding_size] = '\0';
    return new_str;
} // qn_str_encode_base64_urlsafe

qn_string qn_str_decode_base64_urlsafe(const char * restrict str, int str_size)
{
    qn_string new_str = NULL;
    int decoding_size = qn_b64_decode_urlsafe(NULL, 0, str, str_size, 0);
    
    if (decoding_size == 0) {
        return "";
    }

    new_str = malloc(decoding_size + 1);
    if (!new_str) {
        qn_err_set_no_enough_memory();
        return NULL;
    }

    qn_b64_decode_urlsafe(new_str, decoding_size, str, str_size, 0);
    new_str[decoding_size] = '\0';
    return new_str;
} // qn_str_decode_base64_urlsafe

#ifdef __cplusplus
}
#endif
