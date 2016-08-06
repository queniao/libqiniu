#include <ctype.h>
#include "qiniu/base/errors.h"
#include "qiniu/http_header_parser.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of HTTP header scanner ----

typedef enum _QN_HTTP_HDR_PRS_STATUS
{
    QN_HTTP_HDR_PARSING_DONE = 0,
    QN_HTTP_HDR_PARSING_KEY,
    QN_HTTP_HDR_PARSING_COLON,
    QN_HTTP_HDR_PARSING_VALUE
} qn_http_hdr_status;

typedef enum _QN_HTTP_HDR_TOKEN
{
    QN_HTTP_HDR_TKN_UNKNOWN = 0,
    QN_HTTP_HDR_TKN_KEY,
    QN_HTTP_HDR_TKN_COLON,
    QN_HTTP_HDR_TKN_VALUE,
    QN_HTTP_HDR_TKN_LINEFEED,
    QN_HTTP_HDR_TKNERR_NEED_MORE_TEXT,
    QN_HTTP_HDR_TKNERR_INVALID_SYNTAX,
    QN_HTTP_HDR_TKNERR_TEXT_TOO_LONG
} qn_http_hdr_token;

typedef struct _QN_HTTP_HDR_SCANNER
{
    const char * buf;
    int buf_size;
    int buf_pos;

    char txt[512];
    int txt_size;
    qn_http_hdr_status sts;
} qn_http_hdr_scanner, *qn_http_hdr_scanner_ptr;

// ---- Definition of HTTP header parser ----

typedef enum _QN_HTTP_HDR_FLAGS
{
    QN_HTTP_HDR_SETTING_LOCAL_HEADER = 0x1
} qn_http_hdr_flags;

struct _QN_HTTP_HEADER_PARSER
{
    qn_string key;
    qn_http_header_ptr hdr;
    qn_http_hdr_flags flags;
    qn_http_hdr_status sts;
    qn_http_hdr_scanner s;
} qn_http_hdr_parser;

qn_http_hdr_parser_ptr qn_http_hdr_prs_create(void)
{
    qn_http_hdr_parser_ptr new_prs = calloc(1, sizeof(qn_http_hdr_parser));
    if (!new_prs) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if
    return new_prs;
}

void qn_http_hdr_prs_destroy(qn_http_hdr_parser_ptr prs)
{
    if (prs) {
        qn_http_hdr_prs_reset(prs);
        free(prs);
    } // if
}

void qn_http_hdr_prs_reset(qn_http_hdr_parser_ptr prs)
{
    if (prs->flags & QN_HTTP_HDR_SETTING_LOCAL_HEADER) qn_http_hdr_destroy(prs->hdr);
    qn_str_destroy(prs->key);

    prs->hdr = NULL;
    prs->key = NULL;
    prs->flags = 0;
    prs->sts = QN_HTTP_HDR_PARSING_DONE;
    prs->s.txt_size = 0;
}

static qn_http_hdr_token qn_http_hdr_prs_scan_key(qn_http_hdr_scanner_ptr s, const char ** txt, int * txt_size)
{
    char ch;
    int begin;

    if (s->txt_size == 0) {
        // Trim spaces before key.
        while (s->buf_pos < s->buf_size) {
            ch = s->buf[s->buf_pos];
            if (ch == '\n') return QN_HTTP_HDR_TKN_LINEFEED;
            if (!isspace(ch)) break;
            s->buf_pos += 1;
        } // while
    } // if

    begin = s->buf_pos;
    while (s->buf_pos < s->buf_size) {
        ch = s->buf[s->buf_pos];
        if (ch == '\n') return QN_HTTP_HDR_TKN_LINEFEED;
        if (ch == ':' || isspace(ch)) {
            if (s->txt_size == 0) {
                *txt = s->buf + begin;
                *txt_size = s->buf_pos - begin;
            } else {
                if (s->buf_pos - begin > sizeof(s->txt) - s->txt_size - 1) return QN_HTTP_HDR_TKNERR_TEXT_TOO_LONG;
                if (s->buf_pos - begin > 0) memcpy(s->txt + s->txt_size, s->buf + begin, s->buf_pos - begin);
                s->txt_size += s->buf_pos - begin;

                *txt = s->txt;
                *txt_size = s->txt_size;
                s->txt_size = 0;
            } // if
            return QN_HTTP_HDR_TKN_KEY;
        } // if
        s->buf_pos += 1;
    } // while

    if (s->buf_pos - begin > sizeof(s->txt) - s->txt_size - 1) return QN_HTTP_HDR_TKNERR_TEXT_TOO_LONG;
    if (s->buf_pos - begin > 0) memcpy(s->txt + s->txt_size, s->buf + begin, s->buf_pos - begin);
    s->txt_size += s->buf_pos - begin;
    return QN_HTTP_HDR_TKNERR_NEED_MORE_TEXT;
}

static qn_http_hdr_token qn_http_hdr_prs_scan_colon(qn_http_hdr_scanner_ptr s, const char ** txt, int * txt_size)
{
    char ch;
    while (s->buf_pos < s->buf_size) {
        ch = s->buf[s->buf_pos];
        if (ch == '\n') return QN_HTTP_HDR_TKN_LINEFEED;
        if (ch == ':') return QN_HTTP_HDR_TKN_COLON;
        if (!isspace(ch)) return QN_HTTP_HDR_TKN_UNKNOWN;
        s->buf_pos += 1;
    } // while
    return QN_HTTP_HDR_TKNERR_NEED_MORE_TEXT;
}

static qn_http_hdr_token qn_http_hdr_prs_scan_value(qn_http_hdr_scanner_ptr s, const char ** txt, int * txt_size)
{
    char ch;
    int begin;
    int end;

    while (s->buf_pos < s->buf_size) {
        ch = s->buf[s->buf_pos];
        if (ch == '\n') return QN_HTTP_HDR_TKN_LINEFEED;
        if (!isspace(ch)) break;
        s->buf_pos += 1;
    } // while

    begin = s->buf_pos;
    while (s->buf_pos < s->buf_size) {
        ch = s->buf[s->buf_pos];
        if (ch == '\n') {
            end = s->buf_pos;
            while (isspace(s->buf[end])) end -= 1;
            if (s->txt_size == 0) {
                *txt = s->buf + begin;
                *txt_size = end - begin;
            } else {
                if (end - begin > sizeof(s->txt) - s->txt_size - 1) return QN_HTTP_HDR_TKNERR_TEXT_TOO_LONG;
                if (end - begin > 0) memcpy(s->txt + s->txt_size, s->buf + begin, end - begin);
                s->txt_size += end - begin;

                *txt = s->txt;
                *txt_size = s->txt_size;
                s->txt_size = 0;
            } // if
            return QN_HTTP_HDR_TKN_VALUE;
        } // if
        s->buf_pos += 1;
    } // while

    end = s->buf_pos;
    while (isspace(s->buf[end])) end -= 1;

    if (end - begin > sizeof(s->txt) - s->txt_size - 1) return QN_HTTP_HDR_TKNERR_TEXT_TOO_LONG;
    if (end - begin > 0) memcpy(s->txt + s->txt_size, s->buf + begin, end - begin);
    s->txt_size += end - begin;
    return QN_HTTP_HDR_TKNERR_NEED_MORE_TEXT;
}

qn_bool qn_http_hdr_prs_parse(qn_http_hdr_parser_ptr prs, const char * buf, int * buf_size, qn_http_header_ptr * hdr)
{
    const char * txt;
    int txt_size;
    qn_http_hdr_token tkn;

    prs->s.buf = buf;
    prs->s.buf_size = *buf_size;
    prs->s.buf_pos = 0;

    if (prs->s.buf_size <= 0) {
        qn_err_set_try_again();
        return qn_false;
    } // if

    if (!prs->hdr) {
        if (*hdr) {
            prs->hdr = *hdr;
        } else {
            if (! (prs->hdr = qn_http_hdr_create())) return qn_false;
            prs->flags |= QN_HTTP_HDR_SETTING_LOCAL_HEADER;
        } // if
    } // if

    do {
        switch (prs->sts) {
            case QN_HTTP_HDR_PARSING_DONE:
            case QN_HTTP_HDR_PARSING_KEY:
                tkn = qn_http_hdr_prs_scan_key(&prs->s, &txt, &txt_size);
                if (tkn == QN_HTTP_HDR_TKN_LINEFEED) {
                    *hdr = prs->hdr;
                    prs->hdr = NULL;
                    *buf_size = prs->s.buf_pos;  // Indicate how many characters consumed.
                    return qn_true;
                } else if (tkn == QN_HTTP_HDR_TKNERR_NEED_MORE_TEXT) {
                    qn_err_set_try_again();
                    return qn_false;
                } else if (tkn != QN_HTTP_HDR_TKN_KEY) {
                    qn_err_http_set_invalid_header_syntax();
                    return qn_false;
                } // if
                prs->key = qn_str_clone(txt, txt_size);
                if (!prs->key) return qn_false;
                prs->sts = QN_HTTP_HDR_PARSING_COLON;

            case QN_HTTP_HDR_PARSING_COLON:
                tkn = qn_http_hdr_prs_scan_colon(&prs->s, &txt, &txt_size);
                if (tkn == QN_HTTP_HDR_TKNERR_NEED_MORE_TEXT) {
                    qn_err_set_try_again();
                    return qn_false;
                } else if (tkn != QN_HTTP_HDR_TKN_COLON) {
                    qn_err_http_set_invalid_header_syntax();
                    return qn_false;
                } // if
                prs->sts = QN_HTTP_HDR_PARSING_VALUE;
                
            case QN_HTTP_HDR_PARSING_VALUE:
                tkn = qn_http_hdr_prs_scan_value(&prs->s, &txt, &txt_size);
                if (tkn == QN_HTTP_HDR_TKNERR_NEED_MORE_TEXT) {
                    qn_err_set_try_again();
                    return qn_false;
                } else if (tkn != QN_HTTP_HDR_TKN_VALUE) {
                    qn_err_http_set_invalid_header_syntax();
                    return qn_false;
                } // if
                if (!qn_http_hdr_set_raw(prs->hdr, qn_str_cstr(prs->key), qn_str_size(prs->key), txt, txt_size)) {
                    qn_str_destroy(prs->key);
                    prs->key = NULL;
                    return qn_false;
                } // if
                prs->sts = QN_HTTP_HDR_PARSING_KEY;
        } // switch
    } while (prs->s.buf_pos < prs->s.buf_size);
    qn_err_set_try_again();
    return qn_false;
}

#ifdef __cplusplus
}
#endif
