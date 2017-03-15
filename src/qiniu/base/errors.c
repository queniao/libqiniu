#include <stdlib.h>
#include <curl/curl.h>

#include "qiniu/base/string.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_ERR_MESSAGE_MAP
{
    qn_err_code_em code;
    const char * message;
} qn_err_message_map_st, *qn_err_message_map_ptr;

static qn_err_message_map_st qn_err_message_maps[] = {
    {QN_ERR_SUCCEED, "Operation succeed"},
    {QN_ERR_OUT_OF_MEMORY, "Out of memory"},
    {QN_ERR_TRY_AGAIN, "Operation would be blocked, try again"},
    {QN_ERR_INVALID_ARGUMENT, "Invalid argument is passed"},
    {QN_ERR_OVERFLOW_UPPER_BOUND, "Integer value is overflow the upper bound"},
    {QN_ERR_OVERFLOW_LOWER_BOUND, "Integer value is overflow the lower bound"},
    {QN_ERR_BAD_UTF8_SEQUENCE, "The string contains a bad UTF-8 sequence"},
    {QN_ERR_OUT_OF_BUFFER, "Out of buffer"},
    {QN_ERR_OUT_OF_CAPACITY, "Out of capacity"},
    {QN_ERR_NO_SUCH_ENTRY, "No such entry to the specified key or name"},
    {QN_ERR_OUT_OF_RANGE, "Out of range"},

    {QN_ERR_JSON_BAD_TEXT_INPUT, "Bad text input of a JSON string is read"},
    {QN_ERR_JSON_TOO_MANY_PARSING_LEVELS, "Parsing too many levels in a piece of JSON text"},
    {QN_ERR_JSON_NEED_MORE_TEXT_INPUT, "Need more text input to parse a JSON object or array"},
    {QN_ERR_JSON_MODIFYING_IMMUTABLE_OBJECT, "Modifying an immutable JSON object"},
    {QN_ERR_JSON_MODIFYING_IMMUTABLE_ARRAY, "Modifying an immutable JSON array"},
    {QN_ERR_JSON_OUT_OF_INDEX, "Out fo index to the array"},

    {QN_ERR_HTTP_INVALID_HEADER_SYNTAX, "Invalid HTTP header syntax"},
    {QN_ERR_HTTP_ADDING_STRING_FIELD_FAILED, "Adding string field to HTTP form failed"},
    {QN_ERR_HTTP_ADDING_FILE_FIELD_FAILED, "Adding file field to HTTP form failed"},
    {QN_ERR_HTTP_ADDING_BUFFER_FIELD_FAILED, "Adding buffer field to HTTP form failed"},

    {QN_ERR_FL_OPENING_FILE_FAILED, "Opening file failed"},
    {QN_ERR_FL_DUPLICATING_FILE_FAILED, "Duplicating file failed"},
    {QN_ERR_FL_READING_FILE_FAILED, "Reading file failed"},
    {QN_ERR_FL_SEEKING_FILE_FAILED, "Seeking file failed"},
    {QN_ERR_FL_INFO_STATING_FILE_INFO_FAILED, "Stating file infomation failed"},

    {QN_ERR_STOR_LACK_OF_AUTHORIZATION_INFORMATION, "Lack of auhorization information like token or put policy"},
    {QN_ERR_STOR_INVALID_RESUMABLE_SESSION_INFORMATION, "Invalid resumable session information"},
    {QN_ERR_STOR_INVALID_LIST_RESULT, "Invalid list result"},
    {QN_ERR_STOR_UPLOAD_ABORTED_BY_FILTER_PRE_CALLBACK, "Upload is aborted by filter pre-callback"},
    {QN_ERR_STOR_UPLOAD_ABORTED_BY_FILTER_POST_CALLBACK, "Upload is aborted by filter post-callback"},
    {QN_ERR_STOR_INVALID_CHUNK_PUT_RESULT, "Invalid chunk put result"},
    {QN_ERR_STOR_API_RETURN_NO_VALUE, "API return no value"},
    {QN_ERR_STOR_LACK_OF_BLOCK_CONTEXT, "Lack of block context"},
    {QN_ERR_STOR_LACK_OF_BLOCK_INFO, "Lack of block information"},
    {QN_ERR_STOR_LACK_OF_FILE_SIZE, "Lack of file size"},
    {QN_ERR_STOR_INVALID_UPLOAD_RESULT, "Invalid upload result"},

    {QN_ERR_ETAG_INITIALIZING_CONTEXT_FAILED, "Failed in initializing a new qetag context"},
    {QN_ERR_ETAG_UPDATING_CONTEXT_FAILED, "Failed in updating the qetag context"},
    {QN_ERR_ETAG_INITIALIZING_BLOCK_FAILED, "Failed in initializing a new qetag block"},
    {QN_ERR_ETAG_UPDATING_BLOCK_FAILED, "Failed in updating the qetag block"},
    {QN_ERR_ETAG_MAKING_DIGEST_FAILED, "Failed in making the qetag digest"},

    {QN_ERR_EASY_INVALID_UPTOKEN, "Got an invalid uptoken"},
    {QN_ERR_EASY_INVALID_PUT_POLICY, "Got an invalid put policy"},

    {QN_ERR_3RDP_GLIBC_ERROR_OCCURRED, "glibc error occurred"},
    {QN_ERR_3RDP_CURL_EASY_ERROR_OCCURRED, "cURL easy error occurred"},
    {QN_ERR_3RDP_OPENSSL_ERROR_OCCURRED, "OpenSSL error occurred"}
};

typedef struct _QN_ERR_MESSAGE
{
    const char * file;
    int line;
    qn_err_code_em code;
    qn_uint32 lib_code;
} qn_err_message_st;

static qn_err_message_st qn_err_msg;

static int qn_err_compare(const void * restrict key, const void * restrict item)
{
    if (*((qn_err_code_em *)key) < ((qn_err_message_map_ptr)item)->code) {
        return -1;
    } // if
    if (*((qn_err_code_em *)key) > ((qn_err_message_map_ptr)item)->code) {
        return 1;
    } // if
    return 0;
}

QN_SDK extern ssize_t qn_err_format_message(char * buf, size_t buf_size)
{
    ssize_t ret;
    ssize_t ret2 = 0;
    char * short_file = posix_strstr(qn_err_msg.file, "src/qiniu/") + 4;

    ret = qn_cs_snprintf(buf, buf_size, "%s:%d %s", short_file, qn_err_msg.line, qn_err_get_message());
    if (0 < ret) {
        switch (qn_err_msg.code) {
            case QN_ERR_3RDP_GLIBC_ERROR_OCCURRED:
                ret2 = qn_cs_snprintf(buf + ret, buf_size - ret, "(%lu:%s)", qn_err_msg.lib_code, strerror(qn_err_msg.lib_code));
                break;
            case QN_ERR_3RDP_CURL_EASY_ERROR_OCCURRED:
                ret2 = qn_cs_snprintf(buf + ret, buf_size - ret, "(%lu:%s)", qn_err_msg.lib_code, curl_easy_strerror(qn_err_msg.lib_code));
                break;
            case QN_ERR_3RDP_OPENSSL_ERROR_OCCURRED:
                ret2 = qn_cs_snprintf(buf + ret, buf_size - ret, "(%lu:%s)", qn_err_msg.lib_code, ERR_error_string(qn_err_msg.lib_code, NULL));
                break;
            default:
                break;
        } // switch
    } // if
    if (ret2 < 0) return ret2;
    return ret + ret2;
}

QN_SDK const char * qn_err_get_message(void)
{
    qn_err_message_map_ptr map = (qn_err_message_map_ptr) bsearch(&qn_err_msg.code, &qn_err_message_maps, sizeof(qn_err_message_maps) / sizeof(qn_err_message_maps[0]), sizeof(qn_err_message_maps[0]), &qn_err_compare);
    return map->message;
}

QN_SDK void qn_err_set_code(qn_err_code_em cd, qn_uint32 lib_cd, const char * restrict file, int line)
{
    qn_err_msg.file = file;
    qn_err_msg.line = line;
    qn_err_msg.code = cd;
    qn_err_msg.lib_code = lib_cd;
}

QN_SDK qn_err_code_em qn_err_get_code(void)
{
    return qn_err_msg.code;
}

#ifdef __cplusplus
}
#endif
