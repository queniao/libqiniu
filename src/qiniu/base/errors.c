#include <stdlib.h>

#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum
{
    QN_ERR_SUCCEED              = 0,
    QN_ERR_NO_ENOUGH_MEMORY     = 1001,
    QN_ERR_TRY_AGAIN            = 1002,
    QN_ERR_INVALID_ARGUMENT     = 1003,
    QN_ERR_OVERFLOW_UPPER_BOUND = 1004,
    QN_ERR_OVERFLOW_LOWER_BOUND = 1005,
    QN_ERR_BAD_UTF8_SEQUENCE    = 1006,
    QN_ERR_NO_ENOUGH_BUFFER     = 1007,
    QN_ERR_JSON_BAD_TEXT_INPUT  = 2001,
    QN_ERR_JSON_TOO_MANY_PARSING_LEVELS = 2002,
    QN_ERR_HTTP_INVALID_HEADER_SYNTAX = 3001,
    QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED = 4001,
    QN_ERR_ETAG_UPDATING_CONTEXT_FAILED = 4002,
    QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED = 4003,
    QN_ERR_ETAG_UPDATING_BLOCK_FAILED = 4004,
    QN_ERR_ETAG_MAKING_DIGEST_FAILED = 4005
};

typedef qn_uint32 qn_err_enum;

static qn_err_enum qn_err_code;

typedef struct _QN_ERROR
{
    qn_uint32 code;
    const char * message;
} qn_error, *qn_error_ptr;

static qn_error qn_errors[] = {
    {QN_ERR_SUCCEED, "Operation succeed"},
    {QN_ERR_NO_ENOUGH_MEMORY, "No enough memory"},
    {QN_ERR_TRY_AGAIN, "Operation would be blocked, try again"},
    {QN_ERR_INVALID_ARGUMENT, "Invalid argument is passed"},
    {QN_ERR_OVERFLOW_UPPER_BOUND, "Integer value is overflow the upper bound"},
    {QN_ERR_OVERFLOW_LOWER_BOUND, "Integer value is overflow the lower bound"},
    {QN_ERR_BAD_UTF8_SEQUENCE, "The string contains a bad UTF-8 sequence"},
    {QN_ERR_NO_ENOUGH_BUFFER, "No enough buffer"},
    {QN_ERR_JSON_BAD_TEXT_INPUT, "Bad text input of a JSON string is read"},
    {QN_ERR_JSON_TOO_MANY_PARSING_LEVELS, "Parsing too many levels in a piece of JSON text"},
    {QN_ERR_HTTP_INVALID_HEADER_SYNTAX, "Invalid HTTP header syntax"},
    {QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED, "Failed in initializing context for qetag calculation"},
    {QN_ERR_ETAG_UPDATING_CONTEXT_FAILED, "Failed in updating context for qetag calculation"},
    {QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED, "Failed in initializing block for qetag calculation"},
    {QN_ERR_ETAG_UPDATING_BLOCK_FAILED, "Failed in updating block for qetag calculation"},
    {QN_ERR_ETAG_MAKING_DIGEST_FAILED, "Failed in making digest for qetag calculation"}
};

static int qn_err_compare(const void * key, const void * item)
{
    if (*((qn_err_enum *)key) < ((qn_error_ptr)item)->code) {
        return -1;
    } // if
    if (*((qn_err_enum *)key) > ((qn_error_ptr)item)->code) {
        return 1;
    } // if
    return 0;
}

const char * qn_err_get_message(void)
{
    qn_error_ptr error = (qn_error_ptr) bsearch(&qn_err_code, &qn_errors, sizeof(qn_errors) / sizeof(qn_errors[0]), sizeof(qn_errors[0]), &qn_err_compare);
    return error->message;
}

void qn_err_set_succeed(void)
{
    qn_err_code = QN_ERR_SUCCEED;
}

void qn_err_set_no_enough_memory(void)
{
    qn_err_code = QN_ERR_NO_ENOUGH_MEMORY;
}

void qn_err_set_try_again(void)
{
    qn_err_code = QN_ERR_TRY_AGAIN;
}

void qn_err_set_invalid_argument(void)
{
    qn_err_code = QN_ERR_INVALID_ARGUMENT;
}

void qn_err_set_overflow_upper_bound(void)
{
    qn_err_code = QN_ERR_OVERFLOW_UPPER_BOUND;
}

void qn_err_set_overflow_lower_bound(void)
{
    qn_err_code = QN_ERR_OVERFLOW_LOWER_BOUND;
}

void qn_err_set_bad_utf8_sequence(void)
{
    qn_err_code = QN_ERR_BAD_UTF8_SEQUENCE;
}

void qn_err_set_no_enough_buffer(void)
{
    qn_err_code = QN_ERR_NO_ENOUGH_BUFFER;
}

void qn_err_json_set_bad_text_input(void)
{
    qn_err_code = QN_ERR_JSON_BAD_TEXT_INPUT;
}

void qn_err_json_set_too_many_parsing_levels(void)
{
    qn_err_code = QN_ERR_JSON_TOO_MANY_PARSING_LEVELS;
}

void qn_err_http_set_invalid_header_syntax(void)
{
    qn_err_code = QN_ERR_HTTP_INVALID_HEADER_SYNTAX;
}

void qn_err_etag_set_initializing_context_failed(void)
{
    qn_err_code = QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED;
}

void qn_err_etag_set_updating_context_failed(void)
{
    qn_err_code = QN_ERR_ETAG_UPDATING_CONTEXT_FAILED;
}

void qn_err_etag_set_initializing_block_failed(void)
{
    qn_err_code = QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED;
}

void qn_err_etag_set_updating_block_failed(void)
{
    qn_err_code = QN_ERR_ETAG_UPDATING_BLOCK_FAILED;
}

void qn_err_etag_set_making_digest_failed(void)
{
    qn_err_code = QN_ERR_ETAG_MAKING_DIGEST_FAILED;
}

// ----

qn_bool qn_err_is_succeed(void)
{
    return (qn_err_code == QN_ERR_SUCCEED);
}

qn_bool qn_err_is_no_enough_memory(void)
{
    return (qn_err_code == QN_ERR_NO_ENOUGH_MEMORY);
}

qn_bool qn_err_is_try_again(void)
{
    return (qn_err_code == QN_ERR_TRY_AGAIN);
}

qn_bool qn_err_is_invalid_argument(void)
{
    return (qn_err_code == QN_ERR_TRY_AGAIN);
}

qn_bool qn_err_is_overflow_upper_bound(void)
{
    return (qn_err_code == QN_ERR_OVERFLOW_UPPER_BOUND);
}

qn_bool qn_err_is_overflow_lower_bound(void)
{
    return (qn_err_code == QN_ERR_OVERFLOW_LOWER_BOUND);
}

qn_bool qn_err_is_bad_utf8_sequence(void)
{
    return (qn_err_code == QN_ERR_BAD_UTF8_SEQUENCE);
}

qn_bool qn_err_is_no_enough_buffer(void)
{
    return (qn_err_code == QN_ERR_NO_ENOUGH_BUFFER);
}

qn_bool qn_err_json_is_bad_text_input(void)
{
    return (qn_err_code == QN_ERR_JSON_BAD_TEXT_INPUT);
}

qn_bool qn_err_json_is_too_many_levels_in_parsing(void)
{
    return (qn_err_code == QN_ERR_JSON_TOO_MANY_PARSING_LEVELS);
}

qn_bool qn_err_http_is_invalid_header_syntax(void)
{
    return (qn_err_code == QN_ERR_HTTP_INVALID_HEADER_SYNTAX);
}

qn_bool qn_err_etag_is_initializing_context_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED);
}

qn_bool qn_err_etag_is_updating_context_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_UPDATING_CONTEXT_FAILED);
}

qn_bool qn_err_etag_is_initializing_block_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED);
}

qn_bool qn_err_etag_is_updating_block_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_UPDATING_BLOCK_FAILED);
}

qn_bool qn_err_etag_is_making_digest_failed(void)
{
    return (qn_err_code == QN_ERR_ETAG_MAKING_DIGEST_FAILED);
}

#ifdef __cplusplus
}
#endif
