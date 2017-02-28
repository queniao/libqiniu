#include <stdlib.h>
#include <assert.h>

#include "qiniu/base/base64.h"

#ifdef __cplusplus
extern "C"
{
#endif

static const char QN_B64_PADDING_CHAR = '=';

static inline void qn_b64_encode_3_bytes(char * map, char d1, char d2, char d3, char * dst)
{ 
    dst[0] = map[(d1 >> 2) & 0x3F];
    dst[1] = map[( (d1 & 0x3) << 4 ) | ( (d2 & 0xF0) >> 4 )];
    dst[2] = map[( (d2 & 0xF) << 2 ) | ( (d3 & 0xC0) >> 6 )];
    dst[3] = map[d3 & 0x3F];
}

static inline void qn_b64_encode_2_bytes(char * map, char d1, char d2, char * dst)
{ 
    dst[0] = map[(d1 >> 2) & 0x3F];
    dst[1] = map[( (d1 & 0x3) << 4 ) | ( (d2 & 0xF0) >> 4 )];
    dst[2] = map[( (d2 & 0xF) << 2 ) | ( (0 & 0xC0) >> 6 )];
}

static inline void qn_b64_encode_1_bytes(char * map, char d1, char * dst)
{ 
    dst[0] = map[(d1 >> 2) & 0x3F];
    dst[1] = map[( (d1 & 0x3) << 4 ) | ( (0 & 0xF0) >> 4 )];
}

static size_t qn_b64_encode(char * restrict encoded_str, size_t encoded_cap, const char * restrict bin, size_t bin_size, qn_uint32 opts, char * map)
{
    size_t i = 0;
    size_t m = 0;
    size_t rem = bin_size;

    while (rem >= 3) {
        qn_b64_encode_3_bytes(map, bin[i], bin[i+1], bin[i+2], &encoded_str[m]);
        i += 3;
        rem -= 3;
        m += 4;
    } // while

    if (rem == 2) {
        qn_b64_encode_2_bytes(map, bin[i], bin[i+1], &encoded_str[m]);
        i += 2;
        rem -= 2;
        m += 3;

        if (opts & QN_B64_APPEND_PADDING) {
            encoded_str[m] = QN_B64_PADDING_CHAR;
            m += 1;
        }
    } else if (rem == 1) {
        qn_b64_encode_1_bytes(map, bin[i], &encoded_str[m]);
        i += 1;
        rem -= 1;
        m += 2;

        if (opts & QN_B64_APPEND_PADDING) {
            encoded_str[m+1] = encoded_str[m] = QN_B64_PADDING_CHAR;
            m += 2;
        }
    } // if

    return m;
}

static inline int qn_b64_calc_ord(char c)
{
    if ('A' <= c && c <= 'Z') {
        return c - 'A';
    }
    if ('a' <= c && c <= 'z') {
        return c - 'a' + 26;
    }
    if ('0' <= c && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '-' || c == '+') {
        return 62;
    }
    if (c == '_' || c == '/') {
        return 63;
    }
    return -1;
}

static inline void qn_b64_decode_4_chars(char c1, char c2, char c3, char c4, char * dst)
{
    dst[0] = ((qn_b64_calc_ord(c1) & 0x3F) << 2) | ((qn_b64_calc_ord(c2) & 0x30) >> 4);
    dst[1] = ((qn_b64_calc_ord(c2) & 0x0F) << 4) | ((qn_b64_calc_ord(c3) & 0x3C) >> 2);
    dst[2] = ((qn_b64_calc_ord(c3) & 0x03) << 6) | (qn_b64_calc_ord(c4) & 0x3F);
}

static inline void qn_b64_decode_3_chars(char c1, char c2, char c3, char * dst)
{
    dst[0] = ((qn_b64_calc_ord(c1) & 0x3F) << 2) | ((qn_b64_calc_ord(c2) & 0x30) >> 4);
    dst[1] = ((qn_b64_calc_ord(c2) & 0x0F) << 4) | ((qn_b64_calc_ord(c3) & 0x3C) >> 2);
}

static inline void qn_b64_decode_2_chars(char c1, char c2, char * dst)
{
    dst[0] = ((qn_b64_calc_ord(c1) & 0x3F) << 2) | ((qn_b64_calc_ord(c2) & 0x30) >> 4);
}

static size_t qn_b64_decode(char * restrict decoded_bin, size_t decoded_cap, const char * str, size_t str_size, qn_uint32 opts)
{
    size_t i = 0;
    size_t m = 0;
    size_t rem = str_size;

    if (str[rem] == QN_B64_PADDING_CHAR) {
        if (str[--rem] == QN_B64_PADDING_CHAR) {
            rem -= 1;
        } // if
    } // if

    while (rem >= 4) {
        qn_b64_decode_4_chars(str[i], str[i+1], str[i+2], str[i+3], &decoded_bin[m]);
        i += 4;
        rem -= 4;
        m += 3;
    } // while
    
    if (rem == 3) {
        qn_b64_decode_3_chars(str[i], str[i+1], str[i+2], &decoded_bin[m]);
        i += 3;
        rem -= 3;
        m += 2;
    } else if (rem == 2) {
        qn_b64_decode_2_chars(str[i], str[i+1], &decoded_bin[m]);
        i += 2;
        rem -= 2;
        m += 1;
    } // if

    return m;
}

static char qn_b64_urlsafe_map[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};

QN_SDK size_t qn_b64_encode_urlsafe(char * restrict encoded_str, size_t encoded_cap, const char * restrict bin, size_t bin_size, qn_uint32 opts)
{
    // Include spaces for two padding chars, but none for the NUL char.
    size_t encoding_size = ((bin_size / 3) * 4) + ((bin_size % 3 > 0) ? 4 : 0);

    if (encoded_str == NULL || encoded_cap == 0) {
        return encoding_size;
    }

    assert(encoding_size <= encoded_cap);
    return qn_b64_encode(encoded_str, encoded_cap, bin, bin_size, opts, qn_b64_urlsafe_map);
}

QN_SDK size_t qn_b64_decode_urlsafe(char * restrict decoded_bin, size_t decoded_cap, const char * restrict str, size_t str_size, qn_uint32 opts)
{
    size_t decoding_size = ((str_size / 4) * 3) + ((str_size % 4 > 0) ? 3 : 0);

    if (decoded_bin == NULL || decoded_cap == 0) {
        return decoding_size;
    }

    assert(decoding_size <= decoded_cap);
    return qn_b64_decode(decoded_bin, decoded_cap, str, str_size, opts);
}

#ifdef __cplusplus
}
#endif
