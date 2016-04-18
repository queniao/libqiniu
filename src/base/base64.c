#include <stdlib.h>

#include "base/base64.h"

#ifdef __cplusplus
extern "C"
{
#endif

static const char QN_B64_PADDING_CHAR = '=';

static inline
void qn_b64_encode_3_bytes(char * map, char d1, char d2, char d3, char * dst)
{ 
    dst[0] = map[(d1 >> 2) & 0x3F];
    dst[1] = map[( (d1 & 0x3) << 4 ) | ( (d2 & 0xF0) >> 4 )];
    dst[2] = map[( (d2 & 0xF) << 2 ) | ( (d3 & 0xC0) >> 6 )];
    dst[3] = map[d3 & 0x3F];
} // qn_b64_encode_3_bytes

static inline
void qn_b64_encode_2_bytes(char * map, char d1, char d2, char * dst)
{ 
    dst[0] = map[(d1 >> 2) & 0x3F];
    dst[1] = map[( (d1 & 0x3) << 4 ) | ( (d2 & 0xF0) >> 4 )];
    dst[2] = map[( (d2 & 0xF) << 2 ) | ( (0 & 0xC0) >> 6 )];
} // qn_b64_encode_2_bytes

static inline
void qn_b64_encode_1_bytes(char * map, char d1, char * dst)
{ 
    dst[0] = map[(d1 >> 2) & 0x3F];
    dst[1] = map[( (d1 & 0x3) << 4 ) | ( (0 & 0xF0) >> 4 )];
} // qn_b64_encode_1_bytes

static
int qn_b64_encode(char * restrict encoded_str, qn_size encoded_size, const char * restrict bin, qn_size bin_size, int opts, char * map)
{
    qn_size i = 0;
    qn_size m = 0;
    qn_size rem = bin_size;

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
        }
    } else if (rem == 1) {
        qn_b64_encode_1_bytes(map, bin[i], &encoded_str[m]);
        i += 1;
        rem -= 1;
        m += 2;

        if (opts & QN_B64_APPEND_PADDING) {
            encoded_str[m+1] = encoded_str[m] = QN_B64_PADDING_CHAR;
        }
    } // if

    return encoded_size;
} // qn_b64_encode

static inline
int qn_b64_calc_ord(char c)
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
} // qn_b64_calc_ord

static inline
void qn_b64_decode_4_chars(char c1, char c2, char c3, char c4, char * dst)
{
    dst[0] = ((qn_b64_calc_ord(c1) & 0x3F) << 2) | ((qn_b64_calc_ord(c2) & 0x30) >> 4);
    dst[1] = ((qn_b64_calc_ord(c2) & 0x0F) << 4) | ((qn_b64_calc_ord(c3) & 0x3C) >> 2);
    dst[2] = ((qn_b64_calc_ord(c3) & 0x03) << 6) | (qn_b64_calc_ord(c4) & 0x3F);
} // qn_b64_decode_4_chars

static inline
void qn_b64_decode_3_chars(char c1, char c2, char c3, char * dst)
{
    dst[0] = ((qn_b64_calc_ord(c1) & 0x3F) << 2) | ((qn_b64_calc_ord(c2) & 0x30) >> 4);
    dst[1] = ((qn_b64_calc_ord(c2) & 0x0F) << 4) | ((qn_b64_calc_ord(c3) & 0x3C) >> 2);
} // qn_b64_decode_3_chars

static inline
void qn_b64_decode_2_chars(char c1, char c2, char * dst)
{
    dst[0] = ((qn_b64_calc_ord(c1) & 0x3F) << 2) | ((qn_b64_calc_ord(c2) & 0x30) >> 4);
} // qn_b64_decode_2_chars

static
int qn_b64_decode(char * restrict bin, qn_size bin_size, const char * encoded_str, qn_size encoded_size, int opts)
{
    int i = 0;
    int m = 0;
    int rem = encoded_size;

    if (encoded_str[rem] == QN_B64_PADDING_CHAR) {
        if (encoded_str[--rem] == QN_B64_PADDING_CHAR) {
            rem -= 1;
        } // if
    } // if

    while (rem >= 4) {
        qn_b64_decode_4_chars(encoded_str[i], encoded_str[i+1], encoded_str[i+2], encoded_str[i+3], &bin[m]);
        i += 4;
        rem -= 4;
        m += 3;
    } // while
    
    if (rem == 3) {
        qn_b64_decode_3_chars(encoded_str[i], encoded_str[i+1], encoded_str[i+2], &bin[m]);
        i += 3;
        rem -= 3;
        m += 2;
    } else if (rem == 2) {
        qn_b64_decode_2_chars(encoded_str[i], encoded_str[i+1], &bin[m]);
        i += 2;
        rem -= 2;
        m += 1;
    } // if

    return bin_size;
} // qn_b64_decode

static char qn_b64_urlsafe_map[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};

int qn_b64_encode_urlsafe(char * restrict encoded_str, qn_size encoded_size, const char * restrict bin, qn_size bin_size, int opts)
{
    if (encoded_str == NULL || encoded_size == 0) {
        return ((bin_size / 3) * 4) + (bin_size % 3 > 0 ? 4 : 0) + 1;
    }
    return qn_b64_encode(encoded_str, encoded_size, bin, bin_size, opts, qn_b64_urlsafe_map);
} // qn_b64_url_encode

int qn_b64_decode_urlsafe(char * restrict bin, qn_size bin_size, const char * restrict encoded_str, qn_size encoded_size, int opts)
{
    if (bin == NULL || bin_size == 0) {
        return ((encoded_size / 4) * 3) + (encoded_size % 4 > 0 ? 3 : 0) + 1;
    }
    return qn_b64_decode(bin, bin_size, encoded_str, encoded_size, opts);
} // qn_b64_url_decode

#ifdef __cplusplus
}
#endif
