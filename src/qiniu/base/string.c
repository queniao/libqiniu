#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <assert.h>

#include "qiniu/base/string.h"
#include "qiniu/base/base64.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of C String ----

QN_SDK qn_string qn_cs_duplicate(const char * restrict s)
{
    return qn_cs_clone(s, strlen(s));
}

QN_SDK qn_string qn_cs_clone(const char * restrict s, qn_size sz)
{
    qn_string new_str = malloc(sz + 1);
    if (!new_str) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    memcpy(new_str, s, sz);
    new_str[sz] = '\0';
    return new_str;
}

QN_SDK qn_string qn_cs_join_list(const char * restrict deli, const char ** restrict ss, int n)
{
    qn_string new_str;
    char * pos;
    qn_size final_size;
    qn_size deli_size;
    qn_size str_size;
    int i;

    if (n == 1) return qn_cs_duplicate(ss[0]);

    deli_size = strlen(deli);

    final_size = 0;
    for (i = 0; i < n; i += 1) final_size += strlen(ss[i]);
    final_size += deli_size * (n - 1);

    pos = new_str = malloc(final_size + 1);
    if (!new_str) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    str_size = strlen(ss[0]);
    memcpy(pos, ss[0], str_size);
    pos += str_size;

    for (i = 1; i < n; i += 1) {
        memcpy(pos, deli, deli_size);
        pos += deli_size;

        str_size = strlen(ss[i]);
        memcpy(pos, ss[i], str_size);
        pos += str_size;
    } // for

    new_str[final_size] = '\0';
    return new_str;
}

QN_SDK qn_string qn_cs_join_va(const char * restrict deli, const char * restrict s1, const char * restrict s2, va_list ap)
{
    va_list cp;
    qn_string new_str;
    qn_string str;
    qn_size final_size;
    qn_size str_size;
    qn_size deli_size = strlen(deli);
    int n;
    char * pos;

    if (!s1) return NULL;
    if (!s2) return qn_cs_duplicate(s1);

    final_size = strlen(s1) + deli_size + strlen(s2);

    n = 0;
    va_copy(cp, ap);
    while ((str = va_arg(cp, qn_string))) {
        final_size += deli_size + strlen(str);
        n += 1;
    } // while
    va_end(cp);

    pos = new_str = malloc(final_size + 1);
    if (!new_str) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    str_size = strlen(s1);
    memcpy(pos, s1, str_size);
    pos += str_size;

    memcpy(pos, deli, deli_size);
    pos += deli_size;

    str_size = strlen(s2);
    memcpy(pos, s2, str_size);
    pos += str_size;

    if (n > 0) {
        va_copy(cp, ap);
        while ((str = va_arg(cp, qn_string))) {
            memcpy(pos, deli, deli_size);
            pos += deli_size;

            str_size = strlen(str);
            memcpy(pos, str, str_size);
            pos += str_size;
        } // while
        va_end(cp);
    } // if

    new_str[final_size] = '\0';
    return new_str; 
}

QN_SDK extern qn_string qn_cs_join_raw_va(const char * restrict deli, const char * restrict s1, qn_size s1_size, const char * restrict s2, qn_size s2_size, va_list ap)
{
    va_list cp;
    qn_string new_str;
    qn_string str;
    qn_size final_size;
    qn_size str_size;
    qn_size deli_size = strlen(deli);
    int n;
    char * pos;

    if (! s1) return NULL;
    if (! s2) return qn_cs_clone(s1, s1_size);

    final_size = s1_size + deli_size + s2_size;

    n = 0;
    va_copy(cp, ap);
    while ((str = va_arg(cp, qn_string))) {
        str_size = va_arg(cp, qn_size);
        final_size += deli_size + str_size;
        n += 1;
    } // while
    va_end(cp);

    pos = new_str = malloc(final_size + 1);
    if (! new_str) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    memcpy(pos, s1, s1_size);
    pos += s1_size;

    memcpy(pos, deli, deli_size);
    pos += deli_size;

    memcpy(pos, s2, s2_size);
    pos += s2_size;

    if (n > 0) {
        va_copy(cp, ap);
        while ((str = va_arg(cp, qn_string))) {
            str_size = va_arg(cp, qn_size);

            memcpy(pos, deli, deli_size);
            pos += deli_size;

            memcpy(pos, str, str_size);
            pos += str_size;
        } // while
        va_end(cp);
    } // if

    new_str[final_size] = '\0';
    return new_str; 
}

QN_SDK qn_string qn_cs_vprintf(const char * restrict format, va_list ap)
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
        qn_err_set_out_of_memory();
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
}

QN_SDK qn_string qn_cs_sprintf(const char * restrict format, ...)
{
    va_list ap;
    qn_string new_str;

    va_start(ap, format);
    new_str = qn_cs_vprintf(format, ap);
    va_end(ap);
    return new_str;
}

#if defined(_MSC_VER)

#if (_MSC_VER < 1400)
#error The version of the MSVC is lower then VC++ 2005.
#endif

QN_SDK int qn_cs_vsnprintf(char * restrict buf, qn_size buf_size, const char * restrict format, va_list ap)
{
    int printed_size;
    char * buf = str;
    qn_size buf_cap = buf_size;

    if (str == NULL || buf_size == 0) {
        buf = 0x1;
        buf_cap = 1;
    } // if

    printed_size = vsnprintf(buf, buf_cap, format, ap);
    return printed_size;
}

#else

QN_SDK int qn_cs_vsnprintf(char * restrict buf, qn_size buf_size, const char * restrict format, va_list ap)
{
    int printed_size;

    printed_size = vsnprintf(buf, buf_size, format, ap);
    if (printed_size < 0) qn_err_3rdp_set_glibc_error_occurred(errno);
    return printed_size;
}

#endif

QN_SDK qn_string qn_cs_encode_base64_urlsafe(const char * restrict bin, qn_size bin_size)
{
    qn_string new_str = NULL;
    qn_size encoding_size = qn_b64_encode_urlsafe(NULL, 0, bin, bin_size, QN_B64_APPEND_PADDING);
    
    if (encoding_size == 0) {
        return "";
    }

    new_str = malloc(encoding_size + 1);
    if (!new_str) {
        qn_err_set_out_of_memory();
        return NULL;
    }

    qn_b64_encode_urlsafe(new_str, encoding_size, bin, bin_size, QN_B64_APPEND_PADDING);
    new_str[encoding_size] = '\0';
    return new_str;
}

QN_SDK qn_string qn_cs_decode_base64_urlsafe(const char * restrict str, qn_size str_size)
{
    qn_string new_str = NULL;
    qn_size decoding_size = qn_b64_decode_urlsafe(NULL, 0, str, str_size, 0);
    
    if (decoding_size == 0) {
        return "";
    }

    new_str = malloc(decoding_size + 1);
    if (!new_str) {
        qn_err_set_out_of_memory();
        return NULL;
    }

    qn_b64_decode_urlsafe(new_str, decoding_size, str, str_size, 0);
    new_str[decoding_size] = '\0';
    return new_str;
}

static const char qn_str_hex_map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

QN_SDK qn_bool qn_cs_percent_encode_check(int c)
{
    if ('a' <= c) {
        if (c <= 'z' || c == '~') {
            return qn_false;
        }
        // { | } DEL or chars > 127
        return qn_true;
    } // if

    if (c <= '9') {
        if ('0' <= c || c == '.' || c == '-') {
            return qn_false;
        } // if
        return qn_true;
    } // if

    if ('A' <= c) {
        if (c <= 'Z' || c == '_') {
            return qn_false;
        }
    } // if
    return qn_true;
}

QN_SDK qn_ssize qn_cs_percent_encode_in_buffer_with_checker(char * restrict buf, qn_size buf_size, const char * restrict bin, qn_size bin_size, qn_cs_percent_encode_check_fn need_to_encode)
{
    int i = 0;
    int m = 0;
    int ret = 0;

    if (! need_to_encode) need_to_encode = &qn_cs_percent_encode_check;

    if (!buf || buf_size <= 0) {
        for (i = 0; i < bin_size; i += 1) {
            if (need_to_encode(bin[i])) {
                if (bin[i] == '%' && (i + 2 < bin_size) && isxdigit(bin[i+1]) && isxdigit(bin[i+2])) {
                    ret += 1;
                } else {
                    ret += 3;
                } // if
            } else {
                ret += 1;
            } // if
        } // for
        return ret;
    } else if (buf_size < bin_size) {
        qn_err_set_out_of_buffer();
        return -1;
    } // if

    for (i = 0; i < bin_size; i += 1) {
        if (need_to_encode(bin[i])) {
            if (bin[i] == '%' && (i + 2 < bin_size) && isxdigit(bin[i+1]) && isxdigit(bin[i+2])) {
                    if (m + 1 > buf_size) {
                        qn_err_set_out_of_buffer();
                        return -1;
                    } // if

                    buf[m++] = bin[i];
            } else {
                if (m + 3 > buf_size) {
                    qn_err_set_out_of_buffer();
                    return -1;
                } // if

                buf[m++] = '%';
                buf[m++] = qn_str_hex_map[(bin[i] >> 4) & 0xF];
                buf[m++] = qn_str_hex_map[bin[i] & 0xF];
            } // if
        } else {
            if (m + 1 > buf_size) {
                qn_err_set_out_of_buffer();
                return -1;
            } // if

            buf[m++] = bin[i];
        }
    } // for
    return m;
}

QN_SDK qn_string qn_cs_percent_encode_with_checker(const char * restrict bin, qn_size bin_size, qn_cs_percent_encode_check_fn need_to_encode)
{
    qn_string new_str = NULL;
    qn_ssize buf_size = qn_cs_percent_encode_in_buffer_with_checker(NULL, 0, bin, bin_size, need_to_encode);

    if (buf_size == bin_size) return qn_cs_clone(bin, bin_size);

    new_str = malloc(buf_size + 1);
    if (!new_str) {
        qn_err_set_out_of_memory();
        return NULL;
    }
    qn_cs_percent_encode_in_buffer_with_checker(new_str, buf_size, bin, bin_size, need_to_encode);
    new_str[buf_size] = '\0';
    return new_str;
}

// ---- Declaration of C String ----

const qn_string qn_str_empty_string = "";

QN_SDK qn_string qn_str_join_list(const char * restrict deli, const qn_string * restrict ss, int n)
{
    qn_string new_str = NULL;
    char * pos = NULL;
    qn_size final_size = 0;
    qn_size deli_size = 0L;
    int i = 0;

    if (n == 1) return qn_str_duplicate(ss[0]);

    deli_size = strlen(deli);

    for (i = 0; i < n; i += 1) {
        final_size += qn_str_size(ss[i]);
    } // for
    final_size += deli_size * (n - 1);

    pos = new_str = malloc(final_size + 1);
    if (!new_str) {
        qn_err_set_out_of_memory();
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

QN_SDK qn_string qn_str_join_va(const char * restrict deli, const qn_string restrict s1, const qn_string restrict s2, va_list ap)
{
    va_list cp;
    qn_string new_str = NULL;
    qn_string str = NULL;
    qn_size final_size = 0;
    qn_size deli_size = strlen(deli);
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
        qn_err_set_out_of_memory();
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

#ifdef __cplusplus
}
#endif

