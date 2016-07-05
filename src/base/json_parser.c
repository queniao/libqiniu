#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "base/errors.h"
#include "base/json_parser.h"

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
    QN_JSON_TKNERR_NO_ENOUGH_MEMORY
} qn_json_token;

typedef struct _QN_JSON_SCANNER
{
    const char * buf;
    qn_size buf_size;
    qn_size pos;
} qn_json_scanner, *qn_json_scanner_ptr;

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

static qn_bool qn_json_unescape_to_utf8(char * cstr, qn_size * i, qn_size * m)
{
    qn_uint32 head_code = 0;
    qn_uint32 tail_code = 0;
    qn_uint32 wch = 0;

#define h1 qn_json_hex_to_dec(cstr[*i+2])
#define h2 qn_json_hex_to_dec(cstr[*i+3])
#define h3 qn_json_hex_to_dec(cstr[*i+4])
#define h4 qn_json_hex_to_dec(cstr[*i+5])

    // Chech if all four hexidecimal characters are valid.
    if (!isxdigit(h1) || !isxdigit(h2) || !isxdigit(h3) || !isxdigit(h4)) {
        qn_err_json_set_bad_text_input();
        return qn_false;
    } // if

    head_code = (h1 << 12) + (h2 << 8) + (h3 << 4) + h4;
    if (head_code <= 127) {
        *i += 6;
        cstr[*m++] = head_code & 0x7F;
        return qn_true;
    } // if

    if (head_code < 0xD800 || 0xDBFF < head_code) {
        qn_err_json_set_bad_text_input();
        return qn_false;
    } // if

    *i += 6;
    if (cstr[*i] != '\\' || cstr[*i+1] != 'u') {
        qn_err_json_set_bad_text_input();
        return qn_false;
    } // if

    tail_code = (h1 << 12) + (h2 << 8) + (h3 << 4) + h4;
    if (tail_code < 0xDC00 || 0xDFFF < tail_code) {
        qn_err_json_set_bad_text_input();
        return qn_false;
    } // if

    *i += 6;
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
    } // if
    return qn_true;

#undef h1
#undef h2
#undef h3
#undef h4
}

static qn_json_token qn_json_scan_string(qn_json_scanner_ptr s, qn_string * txt)
{
    qn_string new_str = NULL;
    char * cstr = NULL;
    qn_size primitive_len = 0;
    qn_size i = 0;
    qn_size m = 0;
    qn_size pos = s->pos + 1; // 跳过起始双引号

    for (; pos < s->buf_size; pos += 1) {
        if (s->buf[pos] == '\\') { m += 1; continue; }
        if (s->buf[pos] == '"' && s->buf[pos - 1] != '\\') { break; }
    } // for
    if (pos == s->buf_size) {
        return QN_JSON_TKNERR_NEED_MORE_TEXT;
    } // if

    pos += 1; // 跳过终止双引号

    primitive_len = pos - s->pos - 2;
    cstr = malloc(primitive_len + 1);
    if (!cstr) {
        qn_err_set_no_enough_memory();
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if
    bzero(cstr, primitive_len + 1);

    memcpy(cstr, s->buf + s->pos + 1, primitive_len);
    if (m > 0) {
        // 处理转义码
        i = 0;
        m = 0;
        while (i < primitive_len) {
            if (cstr[i] == '\\') {
                switch (cstr[i+1]) {
                    case 't': cstr[m++] = '\t'; i += 2; break;
                    case 'n': cstr[m++] = '\n'; i += 2; break;
                    case 'r': cstr[m++] = '\r'; i += 2; break;
                    case 'f': cstr[m++] = '\f'; i += 2; break;
                    case 'v': cstr[m++] = '\v'; i += 2; break;
                    case 'b': cstr[m++] = '\b'; i += 2; break;
                    case 'u':
                        if (!qn_json_unescape_to_utf8(cstr, &i, &m)) {
                            return qn_false;
                        } // if
                        break;

                    default:
                        // Process cases '"', '\\', '/', and other characters.
                        cstr[m++] = cstr[i+1];
                        i += 2;
                        break;
                } // switch
            } else {
                cstr[m++] = cstr[i++];
            } // if
        } // while
        cstr[m] = '\0';
    } else {
        m = primitive_len;
    } // if

    new_str = qn_str_clone(cstr, m);
    free(cstr);

    if (!new_str) {
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if

    *txt = new_str;
    s->pos = pos;
    return QN_JSON_TKN_STRING;
}

static qn_json_token qn_json_scan_number(qn_json_scanner_ptr s, qn_string * txt)
{
    qn_string new_str = NULL;
    qn_json_token tkn = QN_JSON_TKN_INTEGER;
    qn_size primitive_len = 0;
    qn_size pos = s->pos + 1; // 跳过已经被识别的首字符

    for (; pos < s->buf_size; pos += 1) {
        if (s->buf[pos] == '.') {
            if (tkn == QN_JSON_TKN_NUMBER) { break; }
            tkn = QN_JSON_TKN_NUMBER;
            continue;
        } // if
        if (s->buf[pos] < '0' || s->buf[pos] > '9') { break; }
    } // for
    if (pos == s->buf_size) {
        return QN_JSON_TKNERR_NEED_MORE_TEXT;
    } // if

    primitive_len = pos - s->pos;
    new_str = qn_str_clone(s->buf + s->pos, primitive_len);
    if (!new_str) {
        qn_err_set_no_enough_memory();
        return QN_JSON_TKNERR_NO_ENOUGH_MEMORY;
    } // if

    *txt = new_str;
    s->pos = pos;
    return tkn;
}

static qn_json_token qn_json_scan(qn_json_scanner_ptr s, qn_string * txt)
{
    qn_size pos = s->pos;

    for (; pos < s->buf_size && isspace(s->buf[pos]); pos += 1) {
        // do nothing
    } // for
    if (pos == s->buf_size) {
        s->pos = pos;
        return QN_JSON_TKNERR_NEED_MORE_TEXT;
    } // if

    // 此处 s->pos 应指向第一个非空白符的字符
    switch (s->buf[pos]) {
        case ':': s->pos = pos + 1; return QN_JSON_TKN_COLON;
        case ',': s->pos = pos + 1; return QN_JSON_TKN_COMMA;
        case '{': s->pos = pos + 1; return QN_JSON_TKN_OPEN_BRACE;
        case '}': s->pos = pos + 1; return QN_JSON_TKN_CLOSE_BRACE;
        case '[': s->pos = pos + 1; return QN_JSON_TKN_OPEN_BRACKET;
        case ']': s->pos = pos + 1; return QN_JSON_TKN_CLOSE_BRACKET;

        case '"':
            s->pos = pos;
            return qn_json_scan_string(s, txt);

        case '+': case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            s->pos = pos;
            return qn_json_scan_number(s, txt);

        case 't': case 'T':
            if (pos + 4 <= s->buf_size) {
                if (strncmp(s->buf + pos, "true", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_TRUE;
                } // if
                if (strncmp(s->buf + pos, "TRUE", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_TRUE;
                } // if
            } // if
            break;

        case 'f': case 'F':
            if (pos + 5 <= s->buf_size) {
                if (strncmp(s->buf + pos, "false", 5) == 0) {
                    s->pos = pos + 5;
                    return QN_JSON_TKN_FALSE;
                } // if
                if (strncmp(s->buf + pos, "FALSE", 5) == 0) {
                    s->pos = pos + 5;
                    return QN_JSON_TKN_FALSE;
                } // if
            } // if
            break;

        case 'n': case 'N':
            if (pos + 4 <= s->buf_size) {
                if (strncmp(s->buf + pos, "null", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_NULL;
                } // if
                if (strncmp(s->buf + pos, "NULL", 4) == 0) {
                    s->pos = pos + 4;
                    return QN_JSON_TKN_NULL;
                } // if
            } // if
            break;
    } // switch
    return QN_JSON_TKNERR_MALFORMED_TEXT;
}

// ---- Inplementation of parser of JSON ----

static qn_size qn_json_prs_max_levels = 4;

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
    qn_json_ptr elem;
    qn_string key;
    qn_json_prs_status status;
} qn_json_prs_level, *qn_json_prs_level_ptr;

typedef struct _QN_JSON_PARSER
{
    qn_json_scanner scanner;

    int cap;
    int size;
    qn_json_prs_level_ptr lvl;
    qn_json_prs_level init_lvl[3];
} qn_json_parser;

qn_json_parser_ptr qn_json_prs_create(void)
{
    qn_json_parser_ptr new_prs = calloc(1, sizeof(qn_json_parser));
    if (!new_prs) {
        qn_err_set_no_enough_memory();
        return NULL;
    }
    new_prs->lvl = &new_prs->init_lvl[0];
    new_prs->size = 0;
    new_prs->cap = sizeof(new_prs->init_lvl) / sizeof(new_prs->init_lvl[0]);
    return new_prs;
}

void qn_json_prs_destroy(qn_json_parser_ptr prs)
{
    if (prs) {
        while (prs->size > 0) {
            qn_json_destroy(prs->lvl[--prs->size].elem);
        } // while
        if (prs->lvl != &prs->init_lvl[0]) {
            free(prs->lvl);
        } // if
        free(prs);
    } // if
}

void qn_json_prs_reset(qn_json_parser_ptr prs)
{
    if (prs) {
        while (prs->size > 0) {
            qn_json_destroy(prs->lvl[--prs->size].elem);
        } // while
    }
}

static qn_bool qn_json_prs_augment(qn_json_parser_ptr prs)
{
    int new_cap = prs->cap + (prs->cap >> 1);
    qn_json_prs_level_ptr new_lvl = calloc(1, sizeof(qn_json_prs_level) * new_cap);
    if (!new_lvl) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_lvl, prs->lvl, sizeof(qn_json_prs_level) * prs->size);
    if (prs->lvl != &prs->init_lvl[0]) free(prs->lvl);
    prs->lvl = new_lvl;
    prs->cap = new_cap;
    return qn_true;
}

static qn_bool qn_json_prs_push(qn_json_parser_ptr prs, qn_json_ptr elem, qn_json_prs_status status)
{
    if (prs->size == prs->cap && !qn_json_prs_augment(prs)) return qn_false;
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

static qn_bool qn_json_prs_put_in(qn_json_parser_ptr prs, qn_json_token tkn, qn_string txt, qn_json_prs_level_ptr lvl)
{
    qn_integer integer = 0L;
    qn_number number = 0.0L;
    qn_json_ptr new_elem = NULL;
    char * end_txt = NULL;

    switch (tkn) {
        case QN_JSON_TKN_OPEN_BRACE:
            if (prs->size + 1 > qn_json_prs_max_levels) {
                qn_err_json_set_too_many_parsing_levels();
                return qn_false;
            } // if
            if (qn_json_is_object(lvl->elem)) {
                if (! (new_elem = qn_json_create_and_set_object(lvl->elem, lvl->key))) return qn_false;
                if (!qn_json_prs_push(prs, new_elem, QN_JSON_PARSING_KEY)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
            } else {
                if (! (new_elem = qn_json_create_and_push_object(lvl->elem)) ) return qn_false;
                if (!qn_json_prs_push(prs, new_elem, QN_JSON_PARSING_KEY)) return qn_false;
            }
            return qn_true;

        case QN_JSON_TKN_OPEN_BRACKET:
            if (prs->size + 1 > qn_json_prs_max_levels) {
                qn_err_json_set_too_many_parsing_levels();
                return qn_false;
            } // if
            if (qn_json_is_object(lvl->elem)) {
                if (! (new_elem = qn_json_create_and_set_array(lvl->elem, lvl->key))) return qn_false;
                if (!qn_json_prs_push(prs, new_elem, QN_JSON_PARSING_VALUE)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
            } else {
                if (! (new_elem = qn_json_create_and_push_array(lvl->elem)) ) return qn_false;
                if (!qn_json_prs_push(prs, new_elem, QN_JSON_PARSING_VALUE)) return qn_false;
            }
            return qn_true;

        case QN_JSON_TKN_STRING:
            if (qn_json_is_object(lvl->elem)) {
                if (!qn_json_set_string(lvl->elem, lvl->key, txt)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_string(lvl->elem, txt);

        case QN_JSON_TKN_INTEGER:
            integer = strtoll(qn_str_cstr(txt), &end_txt, 10);
            if (end_txt == qn_str_cstr(txt)) {
                qn_err_json_set_bad_text_input();
                return qn_false;
            } // if
            if ((integer == LLONG_MAX || integer == LLONG_MIN) && errno == ERANGE) {
                qn_err_set_overflow_upper_bound();
                return qn_false;
            } // if
            if (qn_json_is_object(lvl->elem)) {
                if (!qn_json_set_integer(lvl->elem, lvl->key, integer)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_integer(lvl->elem, integer);

        case QN_JSON_TKN_NUMBER:
            number = strtold(qn_str_cstr(txt), &end_txt);
            if (end_txt == qn_str_cstr(txt)) {
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
            if (qn_json_is_object(lvl->elem)) {
                if (!qn_json_set_number(lvl->elem, lvl->key, number)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_number(lvl->elem, number);

        case QN_JSON_TKN_TRUE:
            if (qn_json_is_object(lvl->elem)) {
                if (!qn_json_set_boolean(lvl->elem, lvl->key, qn_true)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_boolean(lvl->elem, qn_true);

        case QN_JSON_TKN_FALSE:
            if (qn_json_is_object(lvl->elem)) {
                if (!qn_json_set_boolean(lvl->elem, lvl->key, qn_false)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_boolean(lvl->elem, qn_false);

        case QN_JSON_TKN_NULL:
            if (qn_json_is_object(lvl->elem)) {
                if (!qn_json_set_null(lvl->elem, lvl->key)) return qn_false;
                qn_str_destroy(lvl->key);
                lvl->key = NULL;
                return qn_true;
            }
            return qn_json_push_boolean(lvl->elem, qn_false);

        case QN_JSON_TKNERR_NEED_MORE_TEXT:
            qn_err_set_try_again();
            return qn_false;

        case QN_JSON_TKNERR_NO_ENOUGH_MEMORY:
            qn_err_set_no_enough_memory();
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
    qn_string txt = NULL;

PARSING_NEXT_ELEMENT_IN_THE_OBJECT:
    switch (lvl->status) {
        case QN_JSON_PARSING_KEY:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (tkn == QN_JSON_TKN_CLOSE_BRACE) {
                // The current object element has been parsed.
                lvl->status = QN_JSON_PARSING_DONE;
                return QN_JSON_PARSING_OK;
            } // if

            if (tkn != QN_JSON_TKN_STRING) {
                if (txt) {
                    qn_str_destroy(txt);
                    txt = NULL;
                } // if
                qn_err_json_set_bad_text_input();
                return QN_JSON_PARSING_ERROR;
            } // if

            lvl->key = txt;
            txt = NULL;
            lvl->status = QN_JSON_PARSING_COLON;

        case QN_JSON_PARSING_COLON:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (txt) {
                qn_str_destroy(txt);
                txt = NULL;
            } // if

            if (tkn != QN_JSON_TKN_COLON) {
                qn_err_json_set_bad_text_input();
                return QN_JSON_PARSING_ERROR;
            } // if

            lvl->status = QN_JSON_PARSING_VALUE;

        case QN_JSON_PARSING_VALUE:
            tkn = qn_json_scan(&prs->scanner, &txt);

            // Put the parsed element into the container.
            ret = qn_json_prs_put_in(prs, tkn, txt, lvl);
            if (txt) {
                qn_str_destroy(txt);
                txt = NULL;
            } // if
            if (!ret) return QN_JSON_PARSING_ERROR;

            lvl->status = QN_JSON_PARSING_COMMA;

            // Go to parse the new element on the top of the stack.
            if (tkn == QN_JSON_TKN_OPEN_BRACE || tkn == QN_JSON_TKN_OPEN_BRACKET) return QN_JSON_PARSING_CHILD;

        case QN_JSON_PARSING_COMMA:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (txt) {
                qn_str_destroy(txt);
                txt = NULL;
            } // if

            if (tkn == QN_JSON_TKN_CLOSE_BRACE) {
                // The current object element has been parsed.
                lvl->status = QN_JSON_PARSING_DONE;
                return QN_JSON_PARSING_OK;
            } // if
            if (tkn != QN_JSON_TKN_COMMA) {
                qn_err_json_set_bad_text_input();
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
    qn_string txt = NULL;

PARSING_NEXT_ELEMENT_IN_THE_ARRAY:
    switch (lvl->status) {
        case QN_JSON_PARSING_VALUE:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (tkn == QN_JSON_TKN_CLOSE_BRACKET) {
                // The current array element has been parsed.
                lvl->status = QN_JSON_PARSING_DONE;
                return QN_JSON_PARSING_OK;
            } // if

            // Put the parsed element into the container.
            if (!qn_json_prs_put_in(prs, tkn, txt, lvl)) {
                qn_str_destroy(txt);
                txt = NULL;
                return QN_JSON_PARSING_ERROR;
            } // if

            qn_str_destroy(txt);
            txt = NULL;
            lvl->status = QN_JSON_PARSING_COMMA;

            // Go to parse the new element on the top of the stack.
            if (tkn == QN_JSON_TKN_OPEN_BRACE || tkn == QN_JSON_TKN_OPEN_BRACKET) {
                return QN_JSON_PARSING_CHILD;
            } // if

        case QN_JSON_PARSING_COMMA:
            tkn = qn_json_scan(&prs->scanner, &txt);
            if (tkn == QN_JSON_TKN_CLOSE_BRACKET) {
                // The current array element has been parsed.
                lvl->status = QN_JSON_PARSING_DONE;
                return QN_JSON_PARSING_OK;
            } // if
            if (tkn != QN_JSON_TKN_COMMA) {
                qn_err_json_set_bad_text_input();
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

qn_bool qn_json_prs_parse(qn_json_parser_ptr prs, const char * restrict buf, qn_size * restrict buf_size, qn_json_ptr * root)
{
    qn_json_prs_result parsing_ret;
    qn_json_token tkn = QN_JSON_TKNERR_NEED_MORE_TEXT;
    qn_string txt = NULL;
    qn_json_ptr elem = NULL;
    qn_json_prs_level_ptr lvl = NULL;

    prs->scanner.buf = buf;
    prs->scanner.buf_size = *buf_size;
    prs->scanner.pos = 0;

    if (qn_json_prs_is_empty(prs)) {
        tkn = qn_json_scan(&prs->scanner, &txt);
        if (tkn == QN_JSON_TKN_OPEN_BRACE) {
            if (! (elem = qn_json_create_object()) ) return qn_false;
            if (!qn_json_prs_push(prs, elem, QN_JSON_PARSING_KEY)) return qn_false;
        } else if (tkn == QN_JSON_TKN_OPEN_BRACKET) {
            if (! (elem = qn_json_create_array()) ) return qn_false;
            if (!qn_json_prs_push(prs, elem, QN_JSON_PARSING_VALUE)) return qn_false;
        } else {
            // Not a valid piece of JSON text.
            qn_err_json_set_bad_text_input();
            return qn_false;
        } // if
    } // if

    while (1) {
        lvl = qn_json_prs_top(prs);
        if (qn_json_is_object(lvl->elem)) {
            parsing_ret = qn_json_parse_object(prs, lvl);
        } else {
            parsing_ret = qn_json_parse_array(prs, lvl);
        } // if

        if (parsing_ret == QN_JSON_PARSING_CHILD) continue;
        if (parsing_ret == QN_JSON_PARSING_ERROR) {
            // Failed to parse the current object element.
            *buf_size = prs->scanner.pos;
            return qn_false;
        } // if

        // Pop the parsed element out of the stack.
        qn_json_prs_pop(prs);
        elem = lvl->elem;
        if (qn_json_prs_is_empty(prs)) break; // And it is the root element.
    } // while

    *buf_size = prs->scanner.pos;
    *root = elem;
    return qn_true;
}

qn_size qn_json_prs_get_max_levels(void)
{
    return qn_json_prs_max_levels;
}

qn_bool qn_json_prs_set_max_levels(qn_size count)
{
    if (4 <= qn_json_prs_max_levels && qn_json_prs_max_levels < 64) {
        qn_json_prs_max_levels = count;
    } // if
}

#ifdef __cplusplus
}
#endif
