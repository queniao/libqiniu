#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "qiniu/base/errors.h"
#include "qiniu/base/json_parser.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Implementation of scanner of JSON ----

typedef enum _QN_JSON_TOKEN
{
    QN_JSON_TKN_UNKNOWN = 0,
    QN_JSON_TKN_STRING,
    QN_JSON_TKN_COLON,
    QN_JSON_TKN_COMMA,
    QN_JSON_TKN_INTEGER,
    QN_JSON_TKN_NUMBER,
    QN_JSON_TKN_TRUE,
    QN_JSON_TKN_FALSE,
    QN_JSON_TKN_NULL,
    QN_JSON_TKN_OPEN_BRACE,
    QN_JSON_TKN_CLOSE_BRACE,
    QN_JSON_TKN_OPEN_BRACKET,
    QN_JSON_TKN_CLOSE_BRACKET,
    QN_JSON_TKN_INPUT_END,
    QN_JSON_TKNERR_MALFORMED_TEXT,
    QN_JSON_TKNERR_NEED_MORE_TEXT,
    QN_JSON_TKNERR_TEXT_TOO_LONG
} qn_json_token;

typedef enum
{
    QN_JSON_TKNSTS_STR_CHAR,
    QN_JSON_TKNSTS_STR_ESCAPE_BACKSLASH,
    QN_JSON_TKNSTS_STR_ESCAPE_U,
    QN_JSON_TKNSTS_STR_ESCAPE_C1,
    QN_JSON_TKNSTS_STR_ESCAPE_C2,
    QN_JSON_TKNSTS_STR_ESCAPE_C3,
    QN_JSON_TKNSTS_STR_ESCAPE_C4,

    QN_JSON_TKNSTS_NUM_SIGN,
    QN_JSON_TKNSTS_NUM_INT_DIGITAL,
    QN_JSON_TKNSTS_NUM_DEC_POINT,
    QN_JSON_TKNSTS_NUM_DEC_DIGITAL,

    QN_JSON_TKNSTS_TRUE_T,
    QN_JSON_TKNSTS_TRUE_R,
    QN_JSON_TKNSTS_TRUE_U,

    QN_JSON_TKNSTS_FALSE_F,
    QN_JSON_TKNSTS_FALSE_A,
    QN_JSON_TKNSTS_FALSE_L,
    QN_JSON_TKNSTS_FALSE_S,

    QN_JSON_TKNSTS_NULL_N,
    QN_JSON_TKNSTS_NULL_U,
    QN_JSON_TKNSTS_NULL_L1
} qn_json_tkn_status;

struct _QN_JSON_SCANNER;
typedef struct _QN_JSON_SCANNER * qn_json_scanner_ptr; 

typedef qn_json_token (*qn_json_tkn_scanner)(qn_json_scanner_ptr s, char ** txt, size_t * txt_size);

typedef struct _QN_JSON_SCANNER
{
    const char * buf;
    size_t buf_pos;
    size_t buf_size;

    char txt[1024];
    size_t txt_pos;
    size_t txt_size;

    qn_json_tkn_scanner tkn_scanner;
    qn_json_tkn_status tkn_sts;
} qn_json_scanner;

static inline qn_uint32 qn_json_hex_to_dec(const char ch)
{
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    } // if
    if ('A' <= ch && ch <= 'F') {
        return ch - 'A' + 10;
    } // if
    if ('a' <= ch && ch <= 'f') {
        return ch - 'a' + 10;
    } // if
    return 0;
}

#define h1 qn_json_hex_to_dec(cstr[i+2])
#define h2 qn_json_hex_to_dec(cstr[i+3])
#define h3 qn_json_hex_to_dec(cstr[i+4])
#define h4 qn_json_hex_to_dec(cstr[i+5])

static qn_bool qn_json_unescape_to_ascii(char * cstr, size_t * m)
{
    qn_uint32 head_code;
    size_t i = 0;

    head_code = (h1 << 12) + (h2 << 8) + (h3 << 4) + h4;
    if (head_code <= 127) {
        cstr[*m++] = head_code & 0x7F;
        return qn_true;
    } // if
    return qn_false;
}

static qn_bool qn_json_unescape_to_utf8(char * cstr, size_t * m)
{
    qn_uint32 head_code;
    qn_uint32 tail_code;
    qn_uint32 wch;
    size_t i = 0;

    head_code = (h1 << 12) + (h2 << 8) + (h3 << 4) + h4;
    if (head_code < 0xD800 || 0xDBFF < head_code) {
        qn_err_json_set_bad_text_input();
        return qn_false;
    } // if

    i += 6;
    tail_code = (h1 << 12) + (h2 << 8) + (h3 << 4) + h4;
    if (tail_code < 0xDC00 || 0xDFFF < tail_code) {
        qn_err_json_set_bad_text_input();
        return qn_false;
    } // if

    wch = ((head_code - 0xD800) << 10) + (tail_code - 0xDC00) + 0x10000;
    if (0x80 <= wch && wch <= 0x07FF) {
        // From : 00000000 00000yyy yyzzzzzz
        // To   : 110yyyyy（C0-DF) 10zzzzzz(80-BF）
        cstr[*m] = 0xC0 | ((wch >> 6) & 0x1F);
        cstr[*m+1] = 0x80 | (wch & 0x3F);
        *m += 2;
    } else if ((0x0800 <= wch && wch <= 0xD7FF) || (0xE000 <= wch && wch <= 0xFFFF)) {
        // From : 00000000 xxxxyyyy yyzzzzzz
        // To   : 1110xxxx(E0-EF) 10yyyyyy 10zzzzzz
        cstr[*m] = 0xE0 | ((wch >> 12) & 0x0F);
        cstr[*m+1] = 0x80 | ((wch >> 6) & 0x3F);
        cstr[*m+2] = 0x80 | (wch & 0x3F);
        *m += 3;
    } else if (0x10000 <= wch && wch <= 0x10FFFF) {
        // From : 000wwwxx xxxxyyyy yyzzzzzz
        // To   : 11110www(F0-F7) 10xxxxxx 10yyyyyy 10zzzzzz
        cstr[*m] = 0xF0 | ((wch >> 18) & 0x07);
        cstr[*m+1] = 0x80 | ((wch >> 12) & 0x3F);
        cstr[*m+2] = 0x80 | ((wch >> 6) & 0x3F);
        cstr[*m+3] = 0x80 | (wch & 0x3F);
        *m += 4;
    } else {
        qn_err_json_set_bad_text_input();
        return qn_false;
    } // if
    return qn_true;
}

#undef h4
#undef h3
#undef h2
#undef h1

static qn_json_token qn_json_scan_string(qn_json_scanner_ptr s, char ** txt, size_t * txt_size)
{
    char ch;

    do {
        switch (s->tkn_sts) {
            case QN_JSON_TKNSTS_STR_CHAR:
                do {
                    ch = s->txt[s->txt_size++] = s->buf[s->buf_pos++];
                    if (s->txt_size == sizeof(s->txt) - 1) return QN_JSON_TKNERR_TEXT_TOO_LONG;
                    if (ch == '\\') {
                        s->txt_pos = s->txt_size - 1;
                        s->tkn_sts = QN_JSON_TKNSTS_STR_ESCAPE_BACKSLASH;
                        break;
                    } else if (ch == '"') {
                        s->tkn_scanner = NULL;
                        s->txt[--s->txt_size] = '\0';
                        *txt = s->txt;
                        *txt_size = s->txt_size;
                        return QN_JSON_TKN_STRING;
                    } // if
                } while (s->buf_pos < s->buf_size);
                break;

            case QN_JSON_TKNSTS_STR_ESCAPE_BACKSLASH:
                ch = s->buf[s->buf_pos++];
                if (ch == 'u') {
                    s->txt[s->txt_size++] = ch;
                    if (s->txt_size == sizeof(s->txt) - 1) return QN_JSON_TKNERR_TEXT_TOO_LONG;
                    s->tkn_sts = QN_JSON_TKNSTS_STR_ESCAPE_C1;
                } else {
                    if (ch == 't') {
                        s->txt[s->txt_pos] = '\t';
                    } else if (ch == 'n') {
                        s->txt[s->txt_pos] = '\n';
                    } else if (ch == 'r') {
                        s->txt[s->txt_pos] = '\r';
                    } else if (ch == 'f') {
                        s->txt[s->txt_pos] = '\f';
                    } else if (ch == 'v') {
                        s->txt[s->txt_pos] = '\v';
                    } else if (ch == 'b') {
                        s->txt[s->txt_pos] = '\b';
                    } else {
                        s->txt[s->txt_pos] = ch;
                    } // if
                    s->tkn_sts = QN_JSON_TKNSTS_STR_CHAR;
                } // if
                break;

            case QN_JSON_TKNSTS_STR_ESCAPE_C1:
                ch = s->txt[s->txt_size++] = s->buf[s->buf_pos++];
                if (!isxdigit(ch)) return QN_JSON_TKNERR_MALFORMED_TEXT;
                if (s->txt_size == sizeof(s->txt) - 1) return QN_JSON_TKNERR_TEXT_TOO_LONG;
                s->tkn_sts = QN_JSON_TKNSTS_STR_ESCAPE_C2;
                break;

            case QN_JSON_TKNSTS_STR_ESCAPE_C2:
                ch = s->txt[s->txt_size++] = s->buf[s->buf_pos++];
                if (!isxdigit(ch)) return QN_JSON_TKNERR_MALFORMED_TEXT;
                if (s->txt_size == sizeof(s->txt) - 1) return QN_JSON_TKNERR_TEXT_TOO_LONG;
                s->tkn_sts = QN_JSON_TKNSTS_STR_ESCAPE_C3;
                break;

            case QN_JSON_TKNSTS_STR_ESCAPE_C3:
                ch = s->txt[s->txt_size++] = s->buf[s->buf_pos++];
                if (!isxdigit(ch)) return QN_JSON_TKNERR_MALFORMED_TEXT;
                if (s->txt_size == sizeof(s->txt) - 1) return QN_JSON_TKNERR_TEXT_TOO_LONG;
                if (s->txt_size - s->txt_pos == 6) {
                    if (qn_json_unescape_to_ascii(&s->txt[s->txt_pos], &s->txt_pos)) {
                        s->txt_size = s->txt_pos;
                        s->tkn_sts = QN_JSON_TKNSTS_STR_CHAR;
                    } else {
                        s->tkn_sts = QN_JSON_TKNSTS_STR_ESCAPE_C4;
                    } // if
                } else {
                    if (qn_json_unescape_to_utf8(&s->txt[s->txt_pos], &s->txt_pos)) {
                        s->txt_size = s->txt_pos;
                        s->tkn_sts = QN_JSON_TKNSTS_STR_CHAR;
                    } else {
                        return QN_JSON_TKNERR_MALFORMED_TEXT;
                    } // if
                } // if
                break;

            case QN_JSON_TKNSTS_STR_ESCAPE_C4:
                ch = s->txt[s->txt_size++] = s->buf[s->buf_pos++];
                if (ch != '\\') return QN_JSON_TKNERR_MALFORMED_TEXT;
                if (s->txt_size == sizeof(s->txt) - 1) return QN_JSON_TKNERR_TEXT_TOO_LONG;
                s->tkn_sts = QN_JSON_TKNSTS_STR_ESCAPE_BACKSLASH;
                break;

            default:
                exit(1);
        } // switch
    } while (s->buf_pos < s->buf_size);
    s->tkn_scanner = &qn_json_scan_string;
    return QN_JSON_TKNERR_NEED_MORE_TEXT;
}

static qn_json_token qn_json_scan_number(qn_json_scanner_ptr s, char ** txt, size_t * txt_size)
{
    char ch;

    switch (s->tkn_sts) {
        case QN_JSON_TKNSTS_NUM_SIGN:
            ch = s->txt[s->txt_size++] = s->buf[s->buf_pos++];
            // Since the sign is the first character of a number, don't need to check whether it's to long.
            if (!isdigit(ch)) return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_sts = QN_JSON_TKNSTS_NUM_INT_DIGITAL;
            if (s->buf_pos == s->buf_size) break;

        case QN_JSON_TKNSTS_NUM_INT_DIGITAL:
            while (1) {
                ch = s->txt[s->txt_size++] = s->buf[s->buf_pos++];
                if (s->txt_size == sizeof(s->txt) - 1) return QN_JSON_TKNERR_TEXT_TOO_LONG;
                if (isdigit(ch)) {
                    if (s->buf_pos == s->buf_size) {
                        s->tkn_scanner = &qn_json_scan_number;
                        return QN_JSON_TKNERR_NEED_MORE_TEXT;
                    } // if
                    continue;
                } else if (ch == '.') {
                    s->tkn_sts = QN_JSON_TKNSTS_NUM_DEC_POINT;
                    break;
                } else {
                    s->buf_pos -= 1;
                    s->txt[--s->txt_size] = '\0';
                    s->tkn_scanner = NULL;
                    *txt = s->txt;
                    *txt_size = s->txt_size;
                    return QN_JSON_TKN_INTEGER;
                } // if
            } // while

        case QN_JSON_TKNSTS_NUM_DEC_POINT:
            ch = s->txt[s->txt_size++] = s->buf[s->buf_pos++];
            if (!isdigit(ch)) return QN_JSON_TKNERR_MALFORMED_TEXT;
            if (s->txt_size == sizeof(s->txt) - 1) return QN_JSON_TKNERR_TEXT_TOO_LONG;
            s->tkn_sts = QN_JSON_TKNSTS_NUM_DEC_DIGITAL;
            if (s->buf_pos == s->buf_size) break;

        case QN_JSON_TKNSTS_NUM_DEC_DIGITAL:
            while (1) {
                ch = s->txt[s->txt_size++] = s->buf[s->buf_pos++];
                if (s->txt_size == sizeof(s->txt) - 1) return QN_JSON_TKNERR_TEXT_TOO_LONG;
                if (isdigit(ch)) {
                    if (s->buf_pos == s->buf_size) {
                        s->tkn_scanner = &qn_json_scan_number;
                        return QN_JSON_TKNERR_NEED_MORE_TEXT;
                    } // if
                    continue;
                } else {
                    s->buf_pos -= 1;
                    s->txt[s->txt_size] = '\0';
                    s->tkn_scanner = NULL;
                    *txt = s->txt;
                    *txt_size = s->txt_size;
                    return QN_JSON_TKN_NUMBER;
                } // if
            } // while

        default:
            exit(1);
    } // switch
    s->tkn_scanner = &qn_json_scan_number;
    return QN_JSON_TKNERR_NEED_MORE_TEXT;
}

static qn_json_token qn_json_scan_true(qn_json_scanner_ptr s, char ** txt, size_t * txt_size)
{
    char ch;
    switch (s->tkn_sts) {
        case QN_JSON_TKNSTS_TRUE_T:
            ch = s->buf[s->buf_pos++];
            if (ch != 'r' && ch != 'R') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_sts = QN_JSON_TKNSTS_TRUE_R;
            if (s->buf_pos == s->buf_size) break;

        case QN_JSON_TKNSTS_TRUE_R:
            ch = s->buf[s->buf_pos++];
            if (ch != 'u' && ch != 'U') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_sts = QN_JSON_TKNSTS_TRUE_U;
            if (s->buf_pos == s->buf_size) break;

        case QN_JSON_TKNSTS_TRUE_U:
            ch = s->buf[s->buf_pos++];
            if (ch != 'e' && ch != 'E') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_scanner = NULL;
            return QN_JSON_TKN_TRUE;

        default:
            exit(1);
    } // switch
    s->tkn_scanner = &qn_json_scan_true;
    return QN_JSON_TKNERR_NEED_MORE_TEXT;
}

static qn_json_token qn_json_scan_false(qn_json_scanner_ptr s, char ** txt, size_t * txt_size)
{
    char ch;
    switch (s->tkn_sts) {
        case QN_JSON_TKNSTS_FALSE_F:
            ch = s->buf[s->buf_pos++];
            if (ch != 'a' && ch != 'A') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_sts = QN_JSON_TKNSTS_FALSE_A;
            if (s->buf_pos == s->buf_size) break;

        case QN_JSON_TKNSTS_FALSE_A:
            ch = s->buf[s->buf_pos++];
            if (ch != 'l' && ch != 'L') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_sts = QN_JSON_TKNSTS_FALSE_L;
            if (s->buf_pos == s->buf_size) break;

        case QN_JSON_TKNSTS_FALSE_L:
            ch = s->buf[s->buf_pos++];
            if (ch != 's' && ch != 'S') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_sts = QN_JSON_TKNSTS_FALSE_S;
            if (s->buf_pos == s->buf_size) break;

        case QN_JSON_TKNSTS_FALSE_S:
            ch = s->buf[s->buf_pos++];
            if (ch != 'e' && ch != 'E') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_scanner = NULL;
            return QN_JSON_TKN_FALSE;

        default:
            exit(1);
    } // switch
    s->tkn_scanner = &qn_json_scan_false;
    return QN_JSON_TKNERR_NEED_MORE_TEXT;
}

static qn_json_token qn_json_scan_null(qn_json_scanner_ptr s, char ** txt, size_t * txt_size)
{
    char ch;
    switch (s->tkn_sts) {
        case QN_JSON_TKNSTS_NULL_N:
            ch = s->buf[s->buf_pos++];
            if (ch != 'u' && ch != 'U') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_sts = QN_JSON_TKNSTS_NULL_U;
            if (s->buf_pos == s->buf_size) break;

        case QN_JSON_TKNSTS_NULL_U:
            ch = s->buf[s->buf_pos++];
            if (ch != 'l' && ch != 'L') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_sts = QN_JSON_TKNSTS_NULL_L1;
            if (s->buf_pos == s->buf_size) break;

        case QN_JSON_TKNSTS_NULL_L1:
            ch = s->buf[s->buf_pos++];
            if (ch != 'l' && ch != 'L') return QN_JSON_TKNERR_MALFORMED_TEXT;
            s->tkn_scanner = NULL;
            return QN_JSON_TKN_NULL;

        default:
            exit(1);
    } // switch
    s->tkn_scanner = &qn_json_scan_null;
    return QN_JSON_TKNERR_NEED_MORE_TEXT;
}

static qn_json_token qn_json_scan(qn_json_scanner_ptr s, char ** txt, size_t * txt_size)
{
    if (s->buf_pos >= s->buf_size) return QN_JSON_TKNERR_NEED_MORE_TEXT;
    if (!s->tkn_scanner) {
        while (isspace(s->buf[s->buf_pos])) {
            if (++s->buf_pos == s->buf_size) return QN_JSON_TKNERR_NEED_MORE_TEXT;
        } // while

        // Here s->buf_pos must be pointing to the first nonspace character.
        switch (s->buf[s->buf_pos++]) {
            case ':': return QN_JSON_TKN_COLON;
            case ',': return QN_JSON_TKN_COMMA;
            case '{': return QN_JSON_TKN_OPEN_BRACE;
            case '}': return QN_JSON_TKN_CLOSE_BRACE;
            case '[': return QN_JSON_TKN_OPEN_BRACKET;
            case ']': return QN_JSON_TKN_CLOSE_BRACKET;

            case '"':
                s->tkn_sts = QN_JSON_TKNSTS_STR_CHAR;
                s->txt_size = 0;
                if (s->buf_pos == s->buf_size) {
                    s->tkn_scanner = &qn_json_scan_string;
                    return QN_JSON_TKNERR_NEED_MORE_TEXT;
                } // if
                return qn_json_scan_string(s, txt, txt_size);

            case '+': case '-':
                s->tkn_sts = QN_JSON_TKNSTS_NUM_SIGN;
                s->txt_size = 0;
                s->txt[s->txt_size++] = s->buf[s->buf_pos - 1];
                if (s->buf_pos == s->buf_size) {
                    s->tkn_scanner = &qn_json_scan_number;
                    return QN_JSON_TKNERR_NEED_MORE_TEXT;
                } // if
                return qn_json_scan_number(s, txt, txt_size);

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                s->tkn_sts = QN_JSON_TKNSTS_NUM_INT_DIGITAL;
                s->txt_size = 0;
                s->txt[s->txt_size++] = s->buf[s->buf_pos - 1];
                if (s->buf_pos == s->buf_size) {
                    s->tkn_scanner = &qn_json_scan_number;
                    return QN_JSON_TKNERR_NEED_MORE_TEXT;
                } // if
                return qn_json_scan_number(s, txt, txt_size);

            case 't': case 'T':
                s->tkn_sts = QN_JSON_TKNSTS_TRUE_T;
                if (s->buf_pos == s->buf_size) {
                    s->tkn_scanner = &qn_json_scan_true;
                    return QN_JSON_TKNERR_NEED_MORE_TEXT;
                } // if
                return qn_json_scan_true(s, txt, txt_size);

            case 'f': case 'F':
                s->tkn_sts = QN_JSON_TKNSTS_FALSE_F;
                if (s->buf_pos == s->buf_size) {
                    s->tkn_scanner = &qn_json_scan_false;
                    return QN_JSON_TKNERR_NEED_MORE_TEXT;
                } // if
                return qn_json_scan_false(s, txt, txt_size);

            case 'n': case 'N':
                s->tkn_sts = QN_JSON_TKNSTS_NULL_N;
                if (s->buf_pos == s->buf_size) {
                    s->tkn_scanner = &qn_json_scan_null;
                    return QN_JSON_TKNERR_NEED_MORE_TEXT;
                } // if
                return qn_json_scan_null(s, txt, txt_size);
        } // switch
    } else {
        return (*s->tkn_scanner)(s, txt, txt_size);
    } // if
    return QN_JSON_TKNERR_MALFORMED_TEXT;
}

// ---- Inplementation of parser of JSON ----

static int qn_json_prs_max_levels = 4;

typedef enum _QN_JSON_PRS_STATUS
{
    QN_JSON_PARSING_DONE = 0,
    QN_JSON_PARSING_KEY = 1,
    QN_JSON_PARSING_COLON = 2,
    QN_JSON_PARSING_VALUE = 3,
    QN_JSON_PARSING_COMMA = 4
} qn_json_prs_status;

typedef enum _QN_JSON_PRS_RESULT
{
    QN_JSON_PARSING_OK = 0,
    QN_JSON_PARSING_ERROR = 1,
    QN_JSON_PARSING_CHILD = 2
} qn_json_prs_result;

typedef struct _QN_JSON_PRS_LEVEL
{
    qn_json_class class;
    qn_json_variant elem;
    qn_string key;
    qn_json_prs_status status;
} qn_json_prs_level, *qn_json_prs_level_ptr;

typedef struct _QN_JSON_PARSER
{
    qn_json_scanner scanner;
    qn_json_variant elem;

    int cap;
    int size;
    qn_json_prs_level_ptr lvl;
    qn_json_prs_level init_lvl[3];
} qn_json_parser;

QN_API qn_json_parser_ptr qn_json_prs_create(void)
{
    qn_json_parser_ptr new_prs = calloc(1, sizeof(qn_json_parser));
    if (!new_prs) {
        qn_err_set_out_of_memory();
        return NULL;
    }
    new_prs->lvl = &new_prs->init_lvl[0];
    new_prs->size = 0;
    new_prs->cap = sizeof(new_prs->init_lvl) / sizeof(new_prs->init_lvl[0]);
    return new_prs;
}

QN_API void qn_json_prs_destroy(qn_json_parser_ptr restrict prs)
{
    if (prs) {
        while (prs->size > 0) {
            prs->size -= 1;
            if (prs->lvl[prs->size].class == QN_JSON_OBJECT) {
                qn_json_destroy_object(prs->lvl[prs->size].elem.object);
            } else {
                qn_json_destroy_array(prs->lvl[prs->size].elem.array);
            }
        } // while
        if (prs->lvl != &prs->init_lvl[0]) {
            free(prs->lvl);
        } // if
        free(prs);
    } // if
}

static qn_bool qn_json_prs_augment(qn_json_parser_ptr prs)
{
    int new_cap = prs->cap + (prs->cap >> 1);
    qn_json_prs_level_ptr new_lvl = calloc(1, sizeof(qn_json_prs_level) * new_cap);
    if (!new_lvl) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_lvl, prs->lvl, sizeof(qn_json_prs_level) * prs->size);
    if (prs->lvl != &prs->init_lvl[0]) free(prs->lvl);
    prs->lvl = new_lvl;
    prs->cap = new_cap;
    return qn_true;
}

static qn_bool qn_json_prs_push(qn_json_parser_ptr prs, qn_json_class cls, qn_json_variant elem, qn_json_prs_status status)
{
    if (prs->size == prs->cap && !qn_json_prs_augment(prs)) return qn_false;
    prs->lvl[prs->size].class = cls;
    prs->lvl[prs->size].elem = elem;
    prs->lvl[prs->size].key = NULL;
    prs->lvl[prs->size].status = status;
    prs->size += 1;
    return qn_true;
}

static inline void qn_json_prs_pop(qn_json_parser_ptr prs)
{
    if (prs->size > 0) prs->size -= 1;
}

static inline qn_json_prs_level_ptr qn_json_prs_top(qn_json_parser_ptr prs)
{
    return (prs->size == 0) ? NULL : &prs->lvl[prs->size - 1];
}

static inline qn_bool qn_json_prs_is_empty(qn_json_parser_ptr prs)
{
    return (prs->size == 0);
}

static qn_bool qn_json_prs_put_in(qn_json_parser_ptr prs, qn_json_token tkn, char * txt, size_t txt_size, qn_json_prs_level_ptr lvl)
{
    qn_integer integer = 0L;
    qn_number number = 0.0L;
    qn_json_variant new_elem;
    qn_json_object_ptr new_obj = NULL;
    qn_json_array_ptr new_arr = NULL;
    char * end_txt = NULL;

    switch (tkn) {
        case QN_JSON_TKN_OPEN_BRACE:
            if (prs->size + 1 > qn_json_prs_max_levels) {
                qn_err_json_set_too_many_parsing_levels();
                return qn_false;
            } // if
            if (lvl->class == QN_JSON_OBJECT) {
                if (! (new_obj = qn_json_create_and_set_object(lvl->elem.object, lvl->key))) return qn_false;
                new_elem.object = new_obj;
                if (!qn_json_prs_push(prs, QN_JSON_OBJECT, new_elem, QN_JSON_PARSING_KEY)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
            } else {
                if (! (new_obj = qn_json_create_and_push_object(lvl->elem.array)) ) return qn_false;
                new_elem.object = new_obj;
                if (!qn_json_prs_push(prs, QN_JSON_OBJECT, new_elem, QN_JSON_PARSING_KEY)) return qn_false;
            }
            return qn_true;

        case QN_JSON_TKN_OPEN_BRACKET:
            if (prs->size + 1 > qn_json_prs_max_levels) {
                qn_err_json_set_too_many_parsing_levels();
                return qn_false;
            } // if
            if (lvl->class == QN_JSON_OBJECT) {
                if (! (new_arr = qn_json_create_and_set_array(lvl->elem.object, lvl->key))) return qn_false;
                new_elem.array = new_arr;
                if (!qn_json_prs_push(prs, QN_JSON_ARRAY, new_elem, QN_JSON_PARSING_VALUE)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
            } else {
                if (! (new_arr = qn_json_create_and_push_array(lvl->elem.array)) ) return qn_false;
                new_elem.array = new_arr;
                if (!qn_json_prs_push(prs, QN_JSON_ARRAY, new_elem, QN_JSON_PARSING_VALUE)) return qn_false;
            }
            return qn_true;

        case QN_JSON_TKN_STRING:
            if (lvl->class == QN_JSON_OBJECT) {
                if (!qn_json_set_text(lvl->elem.object, lvl->key, txt, txt_size)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_text(lvl->elem.array, txt, txt_size);

        case QN_JSON_TKN_INTEGER:
            integer = strtoll(txt, &end_txt, 10);
            if (end_txt == txt) {
                qn_err_json_set_bad_text_input();
                return qn_false;
            } // if
            if ((integer == LLONG_MAX || integer == LLONG_MIN) && errno == ERANGE) {
                qn_err_set_overflow_upper_bound();
                return qn_false;
            } // if
            if (lvl->class == QN_JSON_OBJECT) {
                if (!qn_json_set_integer(lvl->elem.object, lvl->key, integer)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_integer(lvl->elem.array, integer);

        case QN_JSON_TKN_NUMBER:
            number = strtold(txt, &end_txt);
            if (end_txt == txt) {
                qn_err_json_set_bad_text_input();
                return qn_false;
            } // if
            if (number >= HUGE_VALL && number <= HUGE_VALL && errno == ERANGE) {
                qn_err_set_overflow_upper_bound();
                return qn_false;
            } // if
            if (number >= 0.0L && number <= 0.0L && errno == ERANGE) {
                qn_err_set_overflow_lower_bound();
                return qn_false;
            } // if
            if (lvl->class == QN_JSON_OBJECT) {
                if (!qn_json_set_number(lvl->elem.object, lvl->key, number)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_number(lvl->elem.array, number);

        case QN_JSON_TKN_TRUE:
            if (lvl->class == QN_JSON_OBJECT) {
                if (!qn_json_set_boolean(lvl->elem.object, lvl->key, qn_true)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_boolean(lvl->elem.array, qn_true);

        case QN_JSON_TKN_FALSE:
            if (lvl->class == QN_JSON_OBJECT) {
                if (!qn_json_set_boolean(lvl->elem.object, lvl->key, qn_false)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_boolean(lvl->elem.array, qn_false);

        case QN_JSON_TKN_NULL:
            if (lvl->class == QN_JSON_OBJECT) {
                if (!qn_json_set_null(lvl->elem.object, lvl->key)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_boolean(lvl->elem.array, qn_false);

        case QN_JSON_TKNERR_NEED_MORE_TEXT:
            qn_err_json_set_need_more_text_input();
            return qn_false;

        default:
            qn_err_json_set_bad_text_input();
            return qn_false;
    } // switch
    return qn_true;
}

static qn_json_prs_result qn_json_parse_object(qn_json_parser_ptr prs, qn_json_prs_level_ptr lvl)
{
    qn_bool ret = qn_false;
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    char * txt = NULL;
    size_t txt_size;

PARSING_NEXT_ELEMENT_IN_THE_OBJECT:
    switch (lvl->status) {
        case QN_JSON_PARSING_KEY:
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
            if (tkn == QN_JSON_TKN_CLOSE_BRACE) {
                // The current object element has been parsed.
                lvl->status = QN_JSON_PARSING_DONE;
                return QN_JSON_PARSING_OK;
            } // if

            if (tkn != QN_JSON_TKN_STRING) {
                if (tkn == QN_JSON_TKNERR_NEED_MORE_TEXT) {
                    qn_err_json_set_need_more_text_input();
                } else {
                    qn_err_json_set_bad_text_input();
                } // if
                return QN_JSON_PARSING_ERROR;
            } // if

            lvl->key = qn_cs_clone(txt, txt_size);
            lvl->status = QN_JSON_PARSING_COLON;

        case QN_JSON_PARSING_COLON:
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);

            if (tkn != QN_JSON_TKN_COLON) {
                if (tkn == QN_JSON_TKNERR_NEED_MORE_TEXT) {
                    qn_err_json_set_need_more_text_input();
                } else {
                    qn_err_json_set_bad_text_input();
                } // if
                return QN_JSON_PARSING_ERROR;
            } // if

            lvl->status = QN_JSON_PARSING_VALUE;

        case QN_JSON_PARSING_VALUE:
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);

            // Put the parsed element into the container.
            ret = qn_json_prs_put_in(prs, tkn, txt, txt_size, lvl);
            if (!ret) return QN_JSON_PARSING_ERROR;

            lvl->status = QN_JSON_PARSING_COMMA;

            // Go to parse the new element on the top of the stack.
            if (tkn == QN_JSON_TKN_OPEN_BRACE || tkn == QN_JSON_TKN_OPEN_BRACKET) return QN_JSON_PARSING_CHILD;

        case QN_JSON_PARSING_COMMA:
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);

            if (tkn == QN_JSON_TKN_CLOSE_BRACE) {
                // The current object element has been parsed.
                lvl->status = QN_JSON_PARSING_DONE;
                return QN_JSON_PARSING_OK;
            } // if
            if (tkn != QN_JSON_TKN_COMMA) {
                if (tkn == QN_JSON_TKNERR_NEED_MORE_TEXT) {
                    qn_err_json_set_need_more_text_input();
                } else {
                    qn_err_json_set_bad_text_input();
                } // if
                return QN_JSON_PARSING_ERROR;
            } // if

            lvl->status = QN_JSON_PARSING_KEY;
            break;

        default:
            assert(lvl->status != QN_JSON_PARSING_DONE);
            return QN_JSON_PARSING_ERROR;
    } // switch

    goto PARSING_NEXT_ELEMENT_IN_THE_OBJECT;
    return QN_JSON_PARSING_ERROR;
}

static qn_json_prs_result qn_json_parse_array(qn_json_parser_ptr prs, qn_json_prs_level_ptr lvl)
{
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    char * txt = NULL;
    size_t txt_size;

PARSING_NEXT_ELEMENT_IN_THE_ARRAY:
    switch (lvl->status) {
        case QN_JSON_PARSING_VALUE:
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
            if (tkn == QN_JSON_TKN_CLOSE_BRACKET) {
                // The current array element has been parsed.
                lvl->status = QN_JSON_PARSING_DONE;
                return QN_JSON_PARSING_OK;
            } // if

            // Put the parsed element into the container.
            if (!qn_json_prs_put_in(prs, tkn, txt, txt_size, lvl)) {
                return QN_JSON_PARSING_ERROR;
            } // if

            lvl->status = QN_JSON_PARSING_COMMA;

            // Go to parse the new element on the top of the stack.
            if (tkn == QN_JSON_TKN_OPEN_BRACE || tkn == QN_JSON_TKN_OPEN_BRACKET) {
                return QN_JSON_PARSING_CHILD;
            } // if

        case QN_JSON_PARSING_COMMA:
            tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
            if (tkn == QN_JSON_TKN_CLOSE_BRACKET) {
                // The current array element has been parsed.
                lvl->status = QN_JSON_PARSING_DONE;
                return QN_JSON_PARSING_OK;
            } // if
            if (tkn != QN_JSON_TKN_COMMA) {
                if (tkn == QN_JSON_TKNERR_NEED_MORE_TEXT) {
                    qn_err_json_set_need_more_text_input();
                } else {
                    qn_err_json_set_bad_text_input();
                } // if
                return QN_JSON_PARSING_ERROR;
            } // if

            lvl->status = QN_JSON_PARSING_VALUE;
            break;

        default:
            assert(
                lvl->status != QN_JSON_PARSING_DONE
                && lvl->status != QN_JSON_PARSING_VALUE
                && lvl->status != QN_JSON_PARSING_COMMA
            );
            return QN_JSON_PARSING_ERROR;
    } // switch

    goto PARSING_NEXT_ELEMENT_IN_THE_ARRAY;
    return QN_JSON_PARSING_ERROR;
}

static qn_bool qn_json_prs_parse(qn_json_parser_ptr prs)
{
    qn_json_prs_result parsing_ret;
    qn_json_prs_level_ptr lvl = NULL;

    do {
        lvl = qn_json_prs_top(prs);
        if (lvl->class == QN_JSON_OBJECT) {
            parsing_ret = qn_json_parse_object(prs, lvl);
        } else {
            parsing_ret = qn_json_parse_array(prs, lvl);
        } // if

        if (parsing_ret == QN_JSON_PARSING_CHILD) continue;
        if (parsing_ret == QN_JSON_PARSING_ERROR) {
            // Failed to parse the current element.
            return qn_false;
        } // if

        // Pop the parsed element out of the stack.
        qn_json_prs_pop(prs);
    } while (!qn_json_prs_is_empty(prs));
    return qn_true;
}

static inline void qn_json_prs_reset(qn_json_parser_ptr restrict prs)
{
    prs->size = 0;
}

QN_API qn_bool qn_json_prs_parse_object(qn_json_parser_ptr restrict prs, const char * restrict buf, size_t * restrict buf_size, qn_json_object_ptr * restrict root)
{
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    char * txt = NULL;
    size_t txt_size;

    prs->scanner.buf = buf;
    prs->scanner.buf_size = *buf_size;
    prs->scanner.buf_pos = 0;

    if (qn_json_prs_is_empty(prs)) {
        tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
        if (tkn == QN_JSON_TKN_OPEN_BRACE) {
            if (*root) {
                prs->elem.object = *root;
            } else if (! (prs->elem.object = qn_json_create_object()) ) {
                return qn_false;
            } // if
            if (!qn_json_prs_push(prs, QN_JSON_OBJECT, prs->elem, QN_JSON_PARSING_KEY)) {
                if (!*root) qn_json_destroy_object(prs->elem.object);
                return qn_false;
            }
        } else {
            // Not a valid piece of JSON text.
            qn_err_json_set_bad_text_input();
            return qn_false;
        } // if
    } // if

    if (!qn_json_prs_parse(prs)) {
        if (!qn_err_json_is_need_more_text_input()) {
            qn_json_prs_reset(prs);
            if (!*root) qn_json_destroy_object(prs->elem.object);
        } // if
        return qn_false;
    } // if
    *root = prs->elem.object;
    *buf_size = prs->scanner.buf_pos;
    return qn_true;
}

QN_API qn_bool qn_json_prs_parse_array(qn_json_parser_ptr restrict prs, const char * restrict buf, size_t * restrict buf_size, qn_json_array_ptr * restrict root)
{
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    char * txt = NULL;
    size_t txt_size;

    prs->scanner.buf = buf;
    prs->scanner.buf_size = *buf_size;
    prs->scanner.buf_pos = 0;

    if (qn_json_prs_is_empty(prs)) {
        tkn = qn_json_scan(&prs->scanner, &txt, &txt_size);
        if (tkn == QN_JSON_TKN_OPEN_BRACKET) {
            if (*root) {
                prs->elem.array = *root;
            } else if (! (prs->elem.array = qn_json_create_array()) ) {
                return qn_false;
            } // if
            if (!qn_json_prs_push(prs, QN_JSON_ARRAY, prs->elem, QN_JSON_PARSING_VALUE)) {
                if (!*root) qn_json_destroy_array(prs->elem.array);
                return qn_false;
            } // if
        } else {
            // Not a valid piece of JSON text.
            qn_err_json_set_bad_text_input();
            return qn_false;
        } // if
    } // if

    if (!qn_json_prs_parse(prs)) {
        if (!qn_err_json_is_need_more_text_input()) {
            qn_json_prs_reset(prs);
            if (!*root) qn_json_destroy_array(prs->elem.array);
        } // if
        return qn_false;
    } // if
    *root = prs->elem.array;
    *buf_size = prs->scanner.buf_pos;
    return qn_true;
}

int qn_json_prs_get_max_levels(void)
{
    return qn_json_prs_max_levels;
}

QN_API void qn_json_prs_set_max_levels(int count)
{
    if (4 <= qn_json_prs_max_levels && qn_json_prs_max_levels < 64) {
        qn_json_prs_max_levels = count;
    } // if
}

#ifdef __cplusplus
}
#endif
