#include "qiniu/base/errors.h"
#include "qiniu/base/json_formatter.h"

#ifdef __cplusplus
extern "C"
{
#endif

//-- Implementation of qn_json_formatter

typedef enum _QN_JSON_FMT_STATUS
{
    QN_JSON_FORMATTING_DONE = 0,
    QN_JSON_FORMATTING_NEXT = 1,
    QN_JSON_FORMATTING_KEY = 2,
    QN_JSON_FORMATTING_COLON = 3,
    QN_JSON_FORMATTING_VALUE = 4,
    QN_JSON_FORMATTING_COMMA = 5,
    QN_JSON_FORMATTING_CLOSURE = 6
} qn_json_fmt_status;

enum
{
    QN_JSON_FMT_ESCAPE_UTF8_STRING = 0x1
};

typedef struct _QN_JSON_FORMATTER {
    int flags;

    char * buf;
    qn_size buf_size;
    qn_size buf_capacity;

    qn_string string;
    qn_size string_pos;
    qn_json_class class;
    qn_json_variant val;

    qn_json_iterator_ptr iterator;
} qn_json_formatter;

#define QN_JSON_FMT_PAGE_SIZE 4096

QN_SDK qn_json_formatter_ptr qn_json_fmt_create(void)
{
    qn_json_formatter_ptr new_fmt = NULL;

    new_fmt = calloc(1, sizeof(qn_json_formatter));
    if (!new_fmt) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_fmt->iterator = qn_json_itr_create();
    if (!new_fmt->iterator) {
        free(new_fmt->buf);
        free(new_fmt);
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_fmt->flags = 0;
    return new_fmt;
}

QN_SDK void qn_json_fmt_destroy(qn_json_formatter_ptr restrict fmt)
{
    if (fmt) {
        qn_json_itr_destroy(fmt->iterator);
        free(fmt);
    } // for
}

QN_SDK void qn_json_fmt_reset(qn_json_formatter_ptr restrict fmt)
{
    qn_json_itr_reset(fmt->iterator);
    fmt->flags = 0;
}

QN_SDK void qn_json_fmt_enable_escape_utf8_string(qn_json_formatter_ptr restrict fmt)
{
    fmt->flags |= QN_JSON_FMT_ESCAPE_UTF8_STRING;
}

QN_SDK void qn_json_fmt_disable_escape_utf8_string(qn_json_formatter_ptr restrict fmt)
{
    fmt->flags &= ~QN_JSON_FMT_ESCAPE_UTF8_STRING;
}

static inline qn_bool qn_json_fmt_putc(qn_json_formatter_ptr fmt, char ch)
{
    if (fmt->buf_size + 1 >= fmt->buf_capacity) {
        qn_err_set_out_of_buffer();
        return qn_false;
    } // if

    fmt->buf[fmt->buf_size++] = ch;
    return qn_true;
}

static qn_bool qn_json_fmt_format_string(qn_json_formatter_ptr fmt)
{
#define c1 (str[pos])
#define c2 (str[pos+1])
#define c3 (str[pos+2])
#define c4 (str[pos+3])
#define head_code (0xD800 + (((wch - 0x10000) & 0xFFC00) >> 10))
#define tail_code (0xDC00 + ((wch - 0x10000) & 0x003FF))

    qn_size free_size;
    qn_size pos = fmt->string_pos;
    qn_size end = qn_str_size(fmt->string);
    const char * str = qn_str_cstr(fmt->string);
    int chars = 0;
    int ret = 0;
    qn_uint32 wch = 0;

    if (pos == 0 && !qn_json_fmt_putc(fmt, '"')) goto FORMATTING_STRING_FAILED;

    while (pos < end) {
        free_size = fmt->buf_capacity - fmt->buf_size;

        if ((c1 & 0x80) == 0 || ((fmt->flags & QN_JSON_FMT_ESCAPE_UTF8_STRING) == 0)) {
            // ASCII range: 0zzzzzzz（00-7F）
            if (c1 == '&') {
                if ((fmt->buf_size + 6) >= fmt->buf_capacity) {
                    qn_err_set_out_of_buffer();
                    goto FORMATTING_STRING_FAILED;
                } // if

                ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "\\u%4X", c1);
                if (ret < 0) goto FORMATTING_STRING_FAILED;
                if (ret >= free_size) {
                    qn_err_set_out_of_buffer();
                    goto FORMATTING_STRING_FAILED;
                } // if

                pos += 1;
                fmt->buf_size += ret;
            } else {
                if (!qn_json_fmt_putc(fmt, c1)) goto FORMATTING_STRING_FAILED;
                pos += 1;
            } // if

            continue;
        } // if

        if ((c1 & 0xE0) == 0xC0) {
            // Check if the c2 is valid.
            if ((end - pos < 2) || (c2 & 0xC0) != 0x80) {
                qn_err_set_bad_utf8_sequence();
                goto FORMATTING_STRING_FAILED;
            } // if

            // From : 110yyyyy（C0-DF) 10zzzzzz(80-BF）
            // To   : 00000000 00000yyy yyzzzzzz
            wch = ((c1 & 0x1F) << 6) | (c2 & 0x3F);
            chars = 2;
        } else if ((c1 & 0xF0) == 0xE0) {
            // Check if the c2 and c3 are valid.
            if ((end - pos < 3) || ((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80)) {
                qn_err_set_bad_utf8_sequence();
                goto FORMATTING_STRING_FAILED;
            } // if

            // From : 1110xxxx(E0-EF) 10yyyyyy 10zzzzzz
            // To   : 00000000 xxxxyyyy yyzzzzzz
            wch = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            chars = 3;
        } else if ((c1 & 0xF8) == 0xF0) {
            // Check if the c2 and c3 and c4 are valid.
            if ((end - pos < 4) || ((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80) || ((c4 & 0xC0) != 0x80)) {
                qn_err_set_bad_utf8_sequence();
                goto FORMATTING_STRING_FAILED;
            } // if

            // From : 11110www(F0-F7) 10xxxxxx 10yyyyyy 10zzzzzz
            // To   : 000wwwxx xxxxyyyy yyzzzzzz
            wch = ((c1 & 0x1F) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
            chars = 4;
        } // if

        if (0xD800 <= wch && wch <= 0xDFFF) {
            qn_err_set_bad_utf8_sequence();
            goto FORMATTING_STRING_FAILED;
        } // if

        if ((fmt->buf_size + 12) >= fmt->buf_capacity) {
            qn_err_set_out_of_buffer();
            goto FORMATTING_STRING_FAILED;
        } // if

        ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "\\u%4X\\u%4X", head_code, tail_code);
        if (ret < 0) goto FORMATTING_STRING_FAILED;
        if (ret >= free_size) {
            qn_err_set_out_of_buffer();
            goto FORMATTING_STRING_FAILED;
        } // if

        pos += chars;
        fmt->buf_size += ret;
    } // while

    if (!qn_json_fmt_putc(fmt, '"')) goto FORMATTING_STRING_FAILED;

    fmt->string = NULL;
    fmt->string_pos = 0;
    return qn_true;

FORMATTING_STRING_FAILED:
    fmt->string_pos = pos;
    return qn_false;

#undef c1
#undef c2
#undef c3
#undef c4
#undef head_code
#undef tail_code
}

static qn_bool qn_json_fmt_format_ordinary(qn_json_formatter_ptr fmt)
{
    int ret = 0;
    const char * str = NULL;
    qn_size free_size = fmt->buf_capacity - fmt->buf_size;

    switch (fmt->class) {
        case QN_JSON_INTEGER:
            ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "%lld", fmt->val.integer);
            break;

        case QN_JSON_NUMBER:
#ifndef QN_CFG_BIG_NUMBERS
            ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "%lf", fmt->val.number);
#else
            ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "%Lf", fmt->val.number);
#endif
            break;

        case QN_JSON_BOOLEAN:
            str = (fmt->val.boolean) ? "true" : "false";
            ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, str);
            break;

        case QN_JSON_NULL:
            ret = qn_cs_snprintf(fmt->buf + fmt->buf_size, free_size, "null");
            break;

        default:
            qn_err_set_invalid_argument();
            return qn_false;
    } // switch

    if (ret < 0) {
        // TODO : Set appropriate errors to accord to each system error accurately.
        qn_err_set_try_again();
        return qn_false;
    } // if

    if (ret >= free_size) {
        // -- Out of buffer.
        qn_err_set_out_of_buffer();
        return qn_false;
    } // if

    // All the output, include the final NUL character, has been written into the buffer.
    fmt->buf_size += ret;
    return qn_true;
}

static int qn_json_fmt_advance_cfn(void * data, qn_json_class cls, qn_json_variant_ptr val)
{
    qn_json_formatter_ptr fmt = (qn_json_formatter_ptr)data;
    if (cls == QN_JSON_UNKNOWN) return QN_JSON_ITR_NO_MORE;
    fmt->class = cls;
    fmt->val = *val;
    return QN_JSON_ITR_OK;
}

static qn_bool qn_json_fmt_format(qn_json_formatter_ptr fmt, char * restrict buf, qn_size * restrict buf_size)
{
    qn_json_object_ptr obj = NULL;
    qn_json_array_ptr arr = NULL;
    int itr_ret = 0;

    while ((obj = qn_json_itr_top_object(fmt->iterator)) || (arr = qn_json_itr_top_array(fmt->iterator))) {
        switch (qn_json_itr_get_status(fmt->iterator)) {
            case QN_JSON_FORMATTING_NEXT:
                itr_ret = qn_json_itr_advance(fmt->iterator, fmt, &qn_json_fmt_advance_cfn);
                if (itr_ret == QN_JSON_ITR_OK) {
                    if (qn_json_itr_done_steps(fmt->iterator) > 1 && !qn_json_fmt_putc(fmt, ',')) {
                        // Output the comma between each element.
                        return qn_false;
                    } // if

                    fmt->string = qn_json_itr_get_key(fmt->iterator);
                    fmt->string_pos = 0;
                    if (fmt->string) {
                        qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_KEY);
                    } else {
                        qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_VALUE);
                    } // if
                } else if (itr_ret == QN_JSON_ITR_NO_MORE) {
                    qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_CLOSURE);
                } // if
                break;

            case QN_JSON_FORMATTING_KEY:
                // Output the key.
                if (!qn_json_fmt_format_string(fmt)) {
                    return qn_false;
                } // if
                qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_COLON);

            case QN_JSON_FORMATTING_COLON:
                // Output the comma between the key and the value.
                if (!qn_json_fmt_putc(fmt, ':')) {
                    return qn_false;
                } // if
                qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_VALUE);

            case QN_JSON_FORMATTING_VALUE:
                if (fmt->class == QN_JSON_OBJECT) {
                    qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_NEXT);
                    if (!qn_json_fmt_putc(fmt, '{') || !qn_json_itr_push_object(fmt->iterator, fmt->val.object)) {
                        return qn_false;
                    } // if
                } else if (fmt->class == QN_JSON_ARRAY) {
                    qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_NEXT);
                    if (!qn_json_fmt_putc(fmt, '[') || !qn_json_itr_push_array(fmt->iterator, fmt->val.array)) {
                        return qn_false;
                    } // if
                } else if (fmt->class == QN_JSON_STRING) {
                    if (!fmt->string) {
                        fmt->string = fmt->val.string;
                        fmt->string_pos = 0;
                    } // if
                    if (!qn_json_fmt_format_string(fmt)) {
                        return qn_false;
                    } // if
                } else {
                    // Output the ordinary element itself.
                    if (!qn_json_fmt_format_ordinary(fmt)) {
                        return qn_false;
                    } // if
                } // if
                qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_NEXT);
                break;

            case QN_JSON_FORMATTING_CLOSURE:
                if (obj) {
                    if (!qn_json_fmt_putc(fmt, '}')) {
                        return qn_false;
                    } // if
                } else {
                    if (!qn_json_fmt_putc(fmt, ']')) {
                        return qn_false;
                    } // if
                } // if
                qn_json_itr_pop(fmt->iterator);
                break;
        } // switch
    } // while

    *buf_size = fmt->buf_size;
    return qn_true;
}

QN_SDK qn_bool qn_json_fmt_format_object(qn_json_formatter_ptr restrict fmt, qn_json_object_ptr restrict root, char * restrict buf, qn_size * restrict buf_size)
{
    qn_bool ret = qn_false;

    fmt->buf = buf;
    fmt->buf_size = 0;
    fmt->buf_capacity = *buf_size;

    if (qn_json_itr_is_empty(fmt->iterator)) {
        // Push the root element into the stack.
        qn_json_itr_push_object(fmt->iterator, root);
        qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_NEXT);
        fmt->buf[fmt->buf_size++] = '{';
    } // if

    ret = qn_json_fmt_format(fmt, buf, buf_size);
    *buf_size = fmt->buf_size;
    return ret;
}

QN_SDK qn_bool qn_json_fmt_format_array(qn_json_formatter_ptr restrict fmt, qn_json_array_ptr restrict root, char * restrict buf, qn_size * restrict buf_size)
{
    qn_bool ret = qn_false;

    fmt->buf = buf;
    fmt->buf_size = 0;
    fmt->buf_capacity = *buf_size;

    if (qn_json_itr_is_empty(fmt->iterator)) {
        // Push the root element into the stack.
        qn_json_itr_push_array(fmt->iterator, root);
        qn_json_itr_set_status(fmt->iterator, QN_JSON_FORMATTING_NEXT);
        fmt->buf[fmt->buf_size++] = '[';
    } // if

    ret = qn_json_fmt_format(fmt, buf, buf_size);
    *buf_size = fmt->buf_size;
    return ret;
}

QN_SDK qn_string qn_json_object_to_string(qn_json_object_ptr restrict root)
{
    qn_json_formatter_ptr fmt = NULL;
    char * buf;
    char * new_buf;
    qn_size capacity = 4096;
    qn_size new_capacity;
    qn_size size = capacity;
    qn_size final_size = 0;

    if (!root) return qn_str_empty_string;

    fmt = qn_json_fmt_create();
    if (!fmt) return NULL;

    buf = malloc(capacity);
    if (!buf) {
        qn_json_fmt_destroy(fmt);
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    while (!qn_json_fmt_format_object(fmt, root, buf + final_size, &size)) {
        if (qn_err_is_out_of_buffer()) {
            final_size += size;

            new_capacity = capacity + (capacity >> 1);
            new_buf = malloc(new_capacity);
            if (!new_buf) {
                free(buf);
                qn_json_fmt_destroy(fmt);
                qn_err_set_out_of_memory();
                return NULL;
            } // if

            memcpy(new_buf, buf, final_size);
            free(buf);
            buf = new_buf;
            capacity = new_capacity;
            size = new_capacity - final_size - 1;
        } else {
            free(buf);
            qn_json_fmt_destroy(fmt);
            return NULL;
        } // if
    } // while
    final_size += size;
    qn_json_fmt_destroy(fmt);

    buf[final_size] = '\0';
    return buf;
}

QN_SDK qn_string qn_json_array_to_string(qn_json_array_ptr restrict root)
{
    qn_json_formatter_ptr fmt = NULL;
    char * buf;
    char * new_buf;
    qn_size capacity = 4096;
    qn_size new_capacity;
    qn_size size = capacity;
    qn_size final_size = 0;

    if (!root) return qn_str_empty_string;

    fmt = qn_json_fmt_create();
    if (!fmt) return NULL;

    buf = malloc(capacity);
    if (!buf) {
        qn_json_fmt_destroy(fmt);
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    while (!qn_json_fmt_format_array(fmt, root, buf + final_size, &size)) {
        if (qn_err_is_out_of_buffer()) {
            final_size += size;

            new_capacity = capacity + (capacity >> 1);
            new_buf = malloc(new_capacity);
            if (!new_buf) {
                free(buf);
                qn_json_fmt_destroy(fmt);
                qn_err_set_out_of_memory();
                return NULL;
            } // if

            memcpy(new_buf, buf, final_size);
            free(buf);
            buf = new_buf;
            capacity = new_capacity;
            size = new_capacity - final_size - 1;
        } else {
            free(buf);
            qn_json_fmt_destroy(fmt);
            return NULL;
        } // if
    } // while
    final_size += size;
    qn_json_fmt_destroy(fmt);

    buf[final_size] = '\0';
    return buf;
}

#ifdef __cplusplus
}
#endif
