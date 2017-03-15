#ifndef __QN_ERRORS_H__
#define __QN_ERRORS_H__ 1

#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _QN_ERR_CODE
{
    QN_ERR_SUCCEED = 0,
    QN_ERR_OUT_OF_MEMORY = 1001,
    QN_ERR_TRY_AGAIN = 1002,
    QN_ERR_INVALID_ARGUMENT = 1003,
    QN_ERR_OVERFLOW_UPPER_BOUND = 1004,
    QN_ERR_OVERFLOW_LOWER_BOUND = 1005,
    QN_ERR_BAD_UTF8_SEQUENCE = 1006,
    QN_ERR_OUT_OF_BUFFER = 1007,
    QN_ERR_OUT_OF_CAPACITY = 1008,
    QN_ERR_NO_SUCH_ENTRY = 1009,
    QN_ERR_OUT_OF_RANGE = 1010,

    QN_ERR_JSON_BAD_TEXT_INPUT  = 2001,
    QN_ERR_JSON_TOO_MANY_PARSING_LEVELS = 2002,
    QN_ERR_JSON_NEED_MORE_TEXT_INPUT = 2003,
    QN_ERR_JSON_MODIFYING_IMMUTABLE_OBJECT = 2004,
    QN_ERR_JSON_MODIFYING_IMMUTABLE_ARRAY = 2005,
    QN_ERR_JSON_OUT_OF_INDEX = 2006,

    QN_ERR_HTTP_INVALID_HEADER_SYNTAX = 3001,
    QN_ERR_HTTP_ADDING_STRING_FIELD_FAILED = 3002,
    QN_ERR_HTTP_ADDING_FILE_FIELD_FAILED = 3003,
    QN_ERR_HTTP_ADDING_BUFFER_FIELD_FAILED = 3004,

    QN_ERR_FL_OPENING_FILE_FAILED = 11001,
    QN_ERR_FL_DUPLICATING_FILE_FAILED = 11002,
    QN_ERR_FL_READING_FILE_FAILED = 11003,
    QN_ERR_FL_SEEKING_FILE_FAILED = 11004,
    QN_ERR_FL_INFO_STATING_FILE_INFO_FAILED = 11101,

    QN_ERR_STOR_LACK_OF_AUTHORIZATION_INFORMATION = 21001,
    QN_ERR_STOR_INVALID_RESUMABLE_SESSION_INFORMATION = 21002,
    QN_ERR_STOR_INVALID_LIST_RESULT = 21003,
    QN_ERR_STOR_UPLOAD_ABORTED_BY_FILTER_PRE_CALLBACK = 21004,
    QN_ERR_STOR_UPLOAD_ABORTED_BY_FILTER_POST_CALLBACK = 21005,
    QN_ERR_STOR_INVALID_CHUNK_PUT_RESULT = 21006,
    QN_ERR_STOR_API_RETURN_NO_VALUE = 21007,
    QN_ERR_STOR_LACK_OF_BLOCK_CONTEXT = 21008,
    QN_ERR_STOR_LACK_OF_BLOCK_INFO = 21009,
    QN_ERR_STOR_LACK_OF_FILE_SIZE = 21010,
    QN_ERR_STOR_INVALID_UPLOAD_RESULT = 21011,

    QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED = 22001,
    QN_ERR_ETAG_UPDATING_CONTEXT_FAILED = 22002,
    QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED = 22003,
    QN_ERR_ETAG_UPDATING_BLOCK_FAILED = 22004,
    QN_ERR_ETAG_MAKING_DIGEST_FAILED = 22005,

    QN_ERR_EASY_INVALID_UPTOKEN = 23001,
    QN_ERR_EASY_INVALID_PUT_POLICY = 23002,

    QN_ERR_3RDP_GLIBC_ERROR_OCCURRED = 101001,
    QN_ERR_3RDP_CURL_EASY_ERROR_OCCURRED = 101002,
    QN_ERR_3RDP_OPENSSL_ERROR_OCCURRED = 101003
} qn_err_code_em;

QN_SDK extern ssize_t qn_err_format_message(char * buf, size_t buf_size);
QN_SDK extern const char * qn_err_get_message(void);

QN_SDK extern void qn_err_set_code(qn_err_code_em cd, qn_uint32 lib_cd, const char * restrict file, int line);
QN_SDK extern qn_err_code_em qn_err_get_code(void);

// ----

#define qn_err_set_succeed() qn_err_set_code(QN_ERR_SUCCEED, 0, __FILE__, __LINE__)
#define qn_err_set_out_of_memory() qn_err_set_code(QN_ERR_OUT_OF_MEMORY, 0, __FILE__, __LINE__)
#define qn_err_set_try_again() qn_err_set_code(QN_ERR_TRY_AGAIN, 0, __FILE__, __LINE__)
#define qn_err_set_invalid_argument() qn_err_set_code(QN_ERR_INVALID_ARGUMENT, 0, __FILE__, __LINE__)
#define qn_err_set_overflow_upper_bound() qn_err_set_code(QN_ERR_OVERFLOW_UPPER_BOUND, 0, __FILE__, __LINE__)
#define qn_err_set_overflow_lower_bound() qn_err_set_code(QN_ERR_OVERFLOW_LOWER_BOUND, 0, __FILE__, __LINE__)
#define qn_err_set_bad_utf8_sequence() qn_err_set_code(QN_ERR_BAD_UTF8_SEQUENCE, 0, __FILE__, __LINE__)
#define qn_err_set_out_of_buffer() qn_err_set_code(QN_ERR_OUT_OF_BUFFER, 0, __FILE__, __LINE__)
#define qn_err_set_out_of_capacity() qn_err_set_code(QN_ERR_OUT_OF_CAPACITY, 0, __FILE__, __LINE__)
#define qn_err_set_no_such_entry() qn_err_set_code(QN_ERR_NO_SUCH_ENTRY, 0, __FILE__, __LINE__)
#define qn_err_set_out_of_range() qn_err_set_code(QN_ERR_OUT_OF_RANGE, 0, __FILE__, __LINE__)

#define qn_err_json_set_bad_text_input() qn_err_set_code(QN_ERR_JSON_BAD_TEXT_INPUT, 0, __FILE__, __LINE__)
#define qn_err_json_set_too_many_parsing_levels() qn_err_set_code(QN_ERR_JSON_TOO_MANY_PARSING_LEVELS, 0, __FILE__, __LINE__)
#define qn_err_json_set_need_more_text_input() qn_err_set_code(QN_ERR_JSON_NEED_MORE_TEXT_INPUT, 0, __FILE__, __LINE__)
#define qn_err_json_set_modifying_immutable_object() qn_err_set_code(QN_ERR_JSON_MODIFYING_IMMUTABLE_OBJECT, 0, __FILE__, __LINE__)
#define qn_err_json_set_modifying_immutable_array() qn_err_set_code(QN_ERR_JSON_MODIFYING_IMMUTABLE_ARRAY, 0, __FILE__, __LINE__)
#define qn_err_json_set_out_of_index() qn_err_set_code(QN_ERR_JSON_OUT_OF_INDEX, 0, __FILE__, __LINE__)

#define qn_err_http_set_invalid_header_syntax() qn_err_set_code(QN_ERR_HTTP_INVALID_HEADER_SYNTAX, 0, __FILE__, __LINE__)
#define qn_err_http_set_adding_string_field_failed() qn_err_set_code(QN_ERR_HTTP_ADDING_STRING_FIELD_FAILED, 0, __FILE__, __LINE__)
#define qn_err_http_set_adding_file_field_failed() qn_err_set_code(QN_ERR_HTTP_ADDING_FILE_FIELD_FAILED, 0, __FILE__, __LINE__)
#define qn_err_http_set_adding_buffer_field_failed() qn_err_set_code(QN_ERR_HTTP_ADDING_BUFFER_FIELD_FAILED, 0, __FILE__, __LINE__)

#define qn_err_fl_set_opening_file_failed() qn_err_set_code(QN_ERR_FL_OPENING_FILE_FAILED, 0, __FILE__, __LINE__)
#define qn_err_fl_set_duplicating_file_failed() qn_err_set_code(QN_ERR_FL_DUPLICATING_FILE_FAILED, 0, __FILE__, __LINE__)
#define qn_err_fl_set_reading_file_failed() qn_err_set_code(QN_ERR_FL_READING_FILE_FAILED, 0, __FILE__, __LINE__)
#define qn_err_fl_set_seeking_file_failed() qn_err_set_code(QN_ERR_FL_SEEKING_FILE_FAILED, 0, __FILE__, __LINE__)

#define qn_err_fl_info_set_stating_file_info_failed() qn_err_set_code(QN_ERR_FL_INFO_STATING_FILE_INFO_FAILED, 0, __FILE__, __LINE__)

#define qn_err_stor_set_lack_of_authorization_information() qn_err_set_code(QN_ERR_STOR_LACK_OF_AUTHORIZATION_INFORMATION, 0, __FILE__, __LINE__)
#define qn_err_stor_set_invalid_resumable_session_information() qn_err_set_code(QN_ERR_STOR_INVALID_RESUMABLE_SESSION_INFORMATION, 0, __FILE__, __LINE__)
#define qn_err_stor_set_invalid_list_result() qn_err_set_code(QN_ERR_STOR_INVALID_LIST_RESULT, 0, __FILE__, __LINE__)
#define qn_err_stor_set_upload_aborted_by_filter_pre_callback() qn_err_set_code(QN_ERR_STOR_UPLOAD_ABORTED_BY_FILTER_PRE_CALLBACK, 0, __FILE__, __LINE__)
#define qn_err_stor_set_upload_aborted_by_filter_post_callback() qn_err_set_code(QN_ERR_STOR_UPLOAD_ABORTED_BY_FILTER_POST_CALLBACK, 0, __FILE__, __LINE__)
#define qn_err_stor_set_invalid_chunk_put_result() qn_err_set_code(QN_ERR_STOR_INVALID_CHUNK_PUT_RESULT, 0, __FILE__, __LINE__)
#define qn_err_stor_set_api_return_no_value() qn_err_set_code(QN_ERR_STOR_API_RETURN_NO_VALUE, 0, __FILE__, __LINE__)
#define qn_err_stor_set_lack_of_block_context() qn_err_set_code(QN_ERR_STOR_LACK_OF_BLOCK_CONTEXT, 0, __FILE__, __LINE__)
#define qn_err_stor_set_lack_of_file_size() qn_err_set_code(QN_ERR_STOR_LACK_OF_FILE_SIZE, 0, __FILE__, __LINE__)
#define qn_err_stor_set_lack_of_block_info() qn_err_set_code(QN_ERR_STOR_LACK_OF_BLOCK_INFO, 0, __FILE__, __LINE__)
#define qn_err_stor_set_invalid_upload_result() qn_err_set_code(QN_ERR_STOR_INVALID_UPLOAD_RESULT, 0, __FILE__, __LINE__)

#define qn_err_etag_set_initializing_context_failed() qn_err_set_code(QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED, 0, __FILE__, __LINE__)
#define qn_err_etag_set_updating_context_failed() qn_err_set_code(QN_ERR_ETAG_UPDATING_CONTEXT_FAILED, 0, __FILE__, __LINE__)
#define qn_err_etag_set_initializing_block_failed() qn_err_set_code(QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED, 0, __FILE__, __LINE__)
#define qn_err_etag_set_updating_block_failed() qn_err_set_code(QN_ERR_ETAG_UPDATING_BLOCK_FAILED, 0, __FILE__, __LINE__)
#define qn_err_etag_set_making_digest_failed() qn_err_set_code(QN_ERR_ETAG_MAKING_DIGEST_FAILED, 0, __FILE__, __LINE__)

#define qn_err_easy_set_invalid_uptoken() qn_err_set_code(QN_ERR_EASY_INVALID_UPTOKEN, 0, __FILE__, __LINE__)
#define qn_err_easy_set_invalid_put_policy() qn_err_set_code(QN_ERR_EASY_INVALID_PUT_POLICY, 0, __FILE__, __LINE__)

#define qn_err_3rdp_set_glibc_error_occurred(lib_cd) qn_err_set_code(QN_ERR_3RDP_GLIBC_ERROR_OCCURRED, lib_cd, __FILE__, __LINE__)
#define qn_err_3rdp_set_curl_easy_error_occurred(lib_cd) qn_err_set_code(QN_ERR_3RDP_CURL_EASY_ERROR_OCCURRED, lib_cd, __FILE__, __LINE__)
#define qn_err_3rdp_set_openssl_error_occurred(lib_cd) qn_err_set_code(QN_ERR_3RDP_OPENSSL_ERROR_OCCURRED, lib_cd, __FILE__, __LINE__)

// ----

static inline qn_bool qn_err_is_succeed(void)
{
    return qn_err_get_code() == QN_ERR_SUCCEED;
}

static inline qn_bool qn_err_is_out_of_memory(void)
{
    return qn_err_get_code() == QN_ERR_OUT_OF_MEMORY;
}

static inline qn_bool qn_err_is_try_again(void)
{
    return qn_err_get_code() == QN_ERR_TRY_AGAIN;
}

static inline qn_bool qn_err_is_invalid_argument(void)
{
    return qn_err_get_code() == QN_ERR_TRY_AGAIN;
}

static inline qn_bool qn_err_is_overflow_upper_bound(void)
{
    return qn_err_get_code() == QN_ERR_OVERFLOW_UPPER_BOUND;
}

static inline qn_bool qn_err_is_overflow_lower_bound(void)
{
    return qn_err_get_code() == QN_ERR_OVERFLOW_LOWER_BOUND;
}

static inline qn_bool qn_err_is_bad_utf8_sequence(void)
{
    return qn_err_get_code() == QN_ERR_BAD_UTF8_SEQUENCE;
}

static inline qn_bool qn_err_is_out_of_buffer(void)
{
    return qn_err_get_code() == QN_ERR_OUT_OF_BUFFER;
}

static inline qn_bool qn_err_is_out_of_capacity(void)
{
    return qn_err_get_code() == QN_ERR_OUT_OF_CAPACITY;
}

static inline qn_bool qn_err_is_no_such_entry(void)
{
    return qn_err_get_code() == QN_ERR_NO_SUCH_ENTRY;
}

static inline qn_bool qn_err_is_out_of_range(void)
{
    return qn_err_get_code() == QN_ERR_OUT_OF_RANGE;
}

static inline qn_bool qn_err_json_is_bad_text_input(void)
{
    return qn_err_get_code() == QN_ERR_JSON_BAD_TEXT_INPUT;
}

static inline qn_bool qn_err_json_is_too_many_parsing_levels(void)
{
    return qn_err_get_code() == QN_ERR_JSON_TOO_MANY_PARSING_LEVELS;
}

static inline qn_bool qn_err_json_is_need_more_text_input(void)
{
    return qn_err_get_code() == QN_ERR_JSON_NEED_MORE_TEXT_INPUT;
}

static inline qn_bool qn_err_json_is_modifying_immutable_object(void)
{
    return qn_err_get_code() == QN_ERR_JSON_MODIFYING_IMMUTABLE_OBJECT;
}

static inline qn_bool qn_err_json_is_modifying_immutable_array(void)
{
    return qn_err_get_code() == QN_ERR_JSON_MODIFYING_IMMUTABLE_ARRAY;
}

static inline qn_bool qn_err_json_is_out_of_index(void)
{
    return qn_err_get_code() == QN_ERR_JSON_OUT_OF_INDEX;
}


static inline qn_bool qn_err_http_is_invalid_header_syntax(void)
{
    return qn_err_get_code() == QN_ERR_HTTP_INVALID_HEADER_SYNTAX;
}

static inline qn_bool qn_err_http_is_adding_string_field_failed(void)
{
    return qn_err_get_code() == QN_ERR_HTTP_ADDING_STRING_FIELD_FAILED;
}

static inline qn_bool qn_err_http_is_adding_file_field_failed(void)
{
    return qn_err_get_code() == QN_ERR_HTTP_ADDING_FILE_FIELD_FAILED;
}

static inline qn_bool qn_err_http_is_adding_buffer_field_failed(void)
{
    return qn_err_get_code() == QN_ERR_HTTP_ADDING_BUFFER_FIELD_FAILED;
}

static inline qn_bool qn_err_fl_is_opening_file_failed(void)
{
    return qn_err_get_code() == QN_ERR_FL_OPENING_FILE_FAILED;
}

static inline qn_bool qn_err_fl_is_duplicating_file_failed(void)
{
    return qn_err_get_code() == QN_ERR_FL_DUPLICATING_FILE_FAILED;
}

static inline qn_bool qn_err_fl_is_reading_file_failed(void)
{
    return qn_err_get_code() == QN_ERR_FL_READING_FILE_FAILED;
}

static inline qn_bool qn_err_fl_is_seeking_file_failed(void)
{
    return qn_err_get_code() == QN_ERR_FL_SEEKING_FILE_FAILED;
}

static inline qn_bool qn_err_fl_info_is_stating_file_info_failed(void)
{
    return qn_err_get_code() == QN_ERR_FL_INFO_STATING_FILE_INFO_FAILED;
}

static inline qn_bool qn_err_stor_is_lack_of_authorization_information(void)
{
    return qn_err_get_code() == QN_ERR_STOR_LACK_OF_AUTHORIZATION_INFORMATION;
}

static inline qn_bool qn_err_stor_is_invalid_resumable_session_information(void)
{
    return qn_err_get_code() == QN_ERR_STOR_INVALID_RESUMABLE_SESSION_INFORMATION;
}

static inline qn_bool qn_err_stor_is_invalid_list_result(void)
{
    return qn_err_get_code() == QN_ERR_STOR_INVALID_LIST_RESULT;
}

static inline qn_bool qn_err_stor_is_upload_aborted_by_filter_pre_callback(void)
{
    return qn_err_get_code() == QN_ERR_STOR_UPLOAD_ABORTED_BY_FILTER_PRE_CALLBACK;
}

static inline qn_bool qn_err_stor_is_upload_aborted_by_filter_post_callback(void)
{
    return qn_err_get_code() == QN_ERR_STOR_UPLOAD_ABORTED_BY_FILTER_POST_CALLBACK;
}

static inline qn_bool qn_err_stor_is_invalid_chunk_put_result(void)
{
    return qn_err_get_code() == QN_ERR_STOR_INVALID_CHUNK_PUT_RESULT;
}

static inline qn_bool qn_err_stor_is_api_return_no_value(void)
{
    return qn_err_get_code() == QN_ERR_STOR_API_RETURN_NO_VALUE;
}

static inline qn_bool qn_err_stor_is_lack_of_block_context(void)
{
    return qn_err_get_code() == QN_ERR_STOR_LACK_OF_BLOCK_CONTEXT;
}

static inline qn_bool qn_err_stor_is_lack_of_block_info(void)
{
    return qn_err_get_code() == QN_ERR_STOR_LACK_OF_BLOCK_INFO;
}

static inline qn_bool qn_err_stor_is_lack_of_file_size(void)
{
    return qn_err_get_code() == QN_ERR_STOR_LACK_OF_FILE_SIZE;
}

static inline qn_bool qn_err_stor_is_invalid_upload_result(void)
{
    return qn_err_get_code() == QN_ERR_STOR_INVALID_UPLOAD_RESULT;
}

static inline qn_bool qn_err_etag_is_initializing_context_failed(void)
{
    return qn_err_get_code() == QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED;
}

static inline qn_bool qn_err_etag_is_updating_context_failed(void)
{
    return qn_err_get_code() == QN_ERR_ETAG_UPDATING_CONTEXT_FAILED;
}

static inline qn_bool qn_err_etag_is_initializing_block_failed(void)
{
    return qn_err_get_code() == QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED;
}

static inline qn_bool qn_err_etag_is_updating_block_failed(void)
{
    return qn_err_get_code() == QN_ERR_ETAG_UPDATING_BLOCK_FAILED;
}

static inline qn_bool qn_err_etag_is_making_digest_failed(void)
{
    return qn_err_get_code() == QN_ERR_ETAG_MAKING_DIGEST_FAILED;
}

static inline qn_bool qn_err_easy_is_invalid_uptoken(void)
{
    return qn_err_get_code() == QN_ERR_EASY_INVALID_UPTOKEN;
}

static inline qn_bool qn_err_easy_is_invalid_put_policy(void)
{
    return qn_err_get_code() == QN_ERR_EASY_INVALID_PUT_POLICY;
}

#ifdef __cplusplus
}
#endif

#endif // __QN_ERRORS_H__
