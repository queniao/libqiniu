#include <assert.h>
#include <curl/curl.h>

#include "qiniu/base/errors.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/base/json_formatter.h"
#include "qiniu/version.h"
#include "qiniu/http.h"
#include "qiniu/http_query.h"
#include "qiniu/storage.h"
#include "qiniu/misc.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of Helper Functions ----

// ---- Definition of Storage ----

typedef struct _QN_STORAGE
{
    qn_http_request_ptr req;
    qn_http_response_ptr resp;
    qn_http_connection_ptr conn;
    qn_http_json_writer_ptr resp_json_wrt;
    qn_json_object_ptr obj_body;
    qn_json_array_ptr arr_body;
} qn_storage;

QN_API qn_storage_ptr qn_stor_create(void)
{
    qn_storage_ptr new_stor = NULL;

    new_stor = calloc(1, sizeof(qn_storage));
    if (!new_stor) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_stor->req = qn_http_req_create();
    if (!new_stor->req) {
        free(new_stor);
        return NULL;
    } // if

    new_stor->resp = qn_http_resp_create();
    if (!new_stor->resp) {
        qn_http_req_destroy(new_stor->req);
        free(new_stor);
        return NULL;
    } // if

    new_stor->conn = qn_http_conn_create();
    if (!new_stor->conn) {
        qn_http_resp_destroy(new_stor->resp);
        qn_http_req_destroy(new_stor->req);
        free(new_stor);
        return NULL;
    } // if

    new_stor->resp_json_wrt = qn_http_json_wrt_create();
    if (!new_stor->resp_json_wrt) {
        qn_http_conn_destroy(new_stor->conn);
        qn_http_resp_destroy(new_stor->resp);
        qn_http_req_destroy(new_stor->req);
        free(new_stor);
        return NULL;
    } // if

    return new_stor;
}

QN_API void qn_stor_destroy(qn_storage_ptr restrict stor)
{
    if (stor) {
        if (stor->obj_body) qn_json_destroy_object(stor->obj_body);
        if (stor->arr_body) qn_json_destroy_array(stor->arr_body);
        qn_http_json_wrt_destroy(stor->resp_json_wrt);
        qn_http_conn_destroy(stor->conn);
        qn_http_resp_destroy(stor->resp);
        qn_http_req_destroy(stor->req);
        free(stor);
    } // if
}

QN_API qn_json_object_ptr qn_stor_get_object_body(const qn_storage_ptr restrict stor)
{
    return stor->obj_body;
}

QN_API qn_json_array_ptr qn_stor_get_array_body(const qn_storage_ptr restrict stor)
{
    return stor->arr_body;
}

QN_API qn_http_hdr_iterator_ptr qn_stor_resp_get_header_iterator(const qn_storage_ptr restrict stor)
{
    return qn_http_resp_get_header_iterator(stor->resp);
}

static qn_bool qn_stor_prepare_common_request_headers(qn_storage_ptr restrict stor)
{
    if (!qn_http_req_set_header(stor->req, "Expect", "")) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Transfer-Encoding", "")) return qn_false;
    if (!qn_http_req_set_header(stor->req, "User-Agent", qn_ver_get_full_string())) return qn_false;
    return qn_true;
}

// ---- Definition of Management ----

static qn_bool qn_stor_prepare_for_managing(qn_storage_ptr restrict stor, const qn_string restrict url, const qn_string restrict hostname, const qn_mac_ptr restrict mac)
{
    qn_bool ret;
    qn_string auth_header;
    qn_string new_acctoken;

    if (!qn_stor_prepare_common_request_headers(stor)) return qn_false;
    if (hostname && !qn_http_req_set_header(stor->req, "Host", qn_str_cstr(hostname))) return qn_false;

    new_acctoken = qn_mac_make_acctoken(mac, url, qn_http_req_body_data(stor->req), qn_http_req_body_size(stor->req));
    if (!new_acctoken) return qn_false;

    auth_header = qn_cs_sprintf("QBox %s", new_acctoken);
    qn_str_destroy(new_acctoken);
    if (!auth_header) return qn_false;

    ret = qn_http_req_set_header(stor->req, "Authorization", auth_header);
    qn_str_destroy(auth_header);
    return ret;
}

static inline void qn_stor_reset(qn_storage_ptr restrict stor)
{
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if
    if (stor->arr_body) {
        qn_json_destroy_array(stor->arr_body);
        stor->arr_body = NULL;
    } // if
}

// ----

typedef struct _QN_STOR_STAT_EXTRA
{
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_stat_extra_st;

QN_API qn_stor_stat_extra_ptr qn_stor_se_create(void)
{
    qn_stor_stat_extra_ptr new_se = calloc(1, sizeof(qn_stor_stat_extra_st));
    if (! new_se) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_se;
}

QN_API void qn_stor_se_destroy(qn_stor_stat_extra_ptr restrict se)
{
    if (se) {
        free(se);
    } // if
}

QN_API void qn_stor_se_reset(qn_stor_stat_extra_ptr restrict se)
{
    memset(se, 0, sizeof(qn_stor_stat_extra_st));
}

QN_API void qn_stor_se_set_region_entry(qn_stor_stat_extra_ptr restrict se, qn_rgn_entry_ptr restrict entry)
{
    se->rgn_entry = entry;
}

static const qn_string qn_stor_make_stat_op(const char * restrict bucket, const char * restrict key)
{
    qn_string op;
    qn_string encoded_uri;

    encoded_uri = qn_misc_encode_uri(bucket, key);
    if (!encoded_uri) return NULL;

    op = qn_cs_sprintf("stat/%.*s", qn_str_size(encoded_uri), qn_str_cstr(encoded_uri));
    qn_str_destroy(encoded_uri);
    return op;
}

/***************************************************************************//**
* @ingroup Storage-Management
*
* Retrieve the meta information of a file.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in] bucket The pointer to a string specifies the bucket where the file
*                    resides.
* @param [in] key The pointer to a string specifies the file itself.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* @retval non-NULL The pointer to a meta information object about the file,
*                  or an error message object (see the REMARK section).
* @retval NULL An application error occurs in stating the file.
*
* @remark The qn_stor_stat() funciton is implemented for two purposes: 1) detect
*         whether a file exists or 2) retrieve the meta information of it.
*
*         If succeeds, the function will return a meta information object
*         contains following fields:
*
*         ```
*             {
*                 "fn-code": <The HTTP code of the response>,
*                 "fn-error": "<The HTTP message of the response>",
*                 "fsize": <File's size in bytes>,
*                 "hash": "<File's hash digest generated by Qiniu-ETAG algorithm>",
*                 "mimeType": "<File's MIME type>",
*                 "putTime": <File's last upload timestamp>
*             }
*         ```
*
*         The `fn-code` and `fn-error` fields hold the HTTP response's status
*         code and message, respectively. They are always returned by the function
*         if the HTTP response returns successfully, no matter the API's operation
*         succeeds or not.
*
*         Other fields are returned only in the case that the API's operation
*         succeeds.
*
*         All HTTP codes and corresponding messages list as follow.
*
*         +-------+-------------------------------------------------------+
*         | Code  | Message                                               |
*         +-------+-------------------------------------------------------+
*         | 200   | OK.                                                   |
*         +-------+-------------------------------------------------------+
*         | 400   | Invalid HTTP request.                                 |
*         +-------+-------------------------------------------------------+
*         | 401   | Bad access token (failed in authorization check).     |
*         +-------+-------------------------------------------------------+
*         | 599   | Server failed due to unknown reason and CONTACT US!   |
*         +-------+-------------------------------------------------------+
*         | 612   | File doesn't exist.                                   |
*         +-------+-------------------------------------------------------+
*         | 631   | Bucket doesn't exist.                                 |
*         +-------+-------------------------------------------------------+
*
*         **NOTE**: The caller MUST NOT destroy the result or error object
*                   because the next call to storage core functions will do
*                   that.
*
*         If fails, the function returns a NULL value and the caller can call
*         qn_err_get_message() to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_stat(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_stat_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string op;
    qn_string url;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(mac);
    assert(bucket);
    assert(key);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } // if

    // ---- Prepare the stat URL.
    op = qn_stor_make_stat_op(bucket, key);
    if (!op) return NULL;

    url = qn_cs_sprintf("%.*s/%.*s", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), qn_str_size(op), qn_str_cstr(op));
    qn_str_destroy(op);
    if (!url) return NULL;

    // ---- Prepare the request and response.
    qn_stor_reset(stor);

    if (!qn_stor_prepare_for_managing(stor, url, rgn_entry->hostname, mac)) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (stor->obj_body = qn_json_create_object())) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the stat action.
    ret = qn_http_conn_get(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);

    if (!ret) return NULL;
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

typedef struct _QN_STOR_COPY_EXTRA
{
    unsigned int force:1;

    qn_rgn_entry_ptr rgn_entry;
} qn_stor_copy_extra_st;

QN_API qn_stor_copy_extra_ptr qn_stor_ce_create(void)
{
    qn_stor_copy_extra_ptr new_ce = calloc(1, sizeof(qn_stor_copy_extra_st));
    if (! new_ce) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_ce;
}

QN_API void qn_stor_ce_destroy(qn_stor_copy_extra_ptr restrict ce)
{
    if (ce) {
        free(ce);
    } // if
}

QN_API void qn_stor_ce_reset(qn_stor_copy_extra_ptr restrict ce)
{
    memset(ce, 0, sizeof(qn_stor_copy_extra_st));
}

QN_API void qn_stor_ce_set_force_overwrite(qn_stor_copy_extra_ptr restrict ce, qn_bool force)
{
    ce->force = (force) ? 1 : 0;
}

QN_API void qn_stor_ce_set_region_entry(qn_stor_copy_extra_ptr restrict ce, qn_rgn_entry_ptr restrict entry)
{
    ce->rgn_entry = entry;
}

static const qn_string qn_stor_make_copy_op(const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key)
{
    qn_string op;
    qn_string encoded_src_uri;
    qn_string encoded_dest_uri;

    encoded_src_uri = qn_misc_encode_uri(src_bucket, src_key);
    if (!encoded_src_uri) return NULL;

    encoded_dest_uri = qn_misc_encode_uri(dest_bucket, dest_key);
    if (!encoded_dest_uri) {
        qn_str_destroy(encoded_src_uri);
        return NULL;
    } // if

    op = qn_cs_sprintf("copy/%.*s/%.*s", qn_str_size(encoded_src_uri), qn_str_cstr(encoded_src_uri), qn_str_size(encoded_dest_uri), qn_str_cstr(encoded_dest_uri));
    qn_str_destroy(encoded_dest_uri);
    qn_str_destroy(encoded_src_uri);
    return op;
}

/***************************************************************************//**
* @ingroup Storage-Management
*
* Copy the source file to the destination file verbatim.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in] src_bucket The pointer to a string specifies the bucket where the
*                        source file resides.
* @param [in] src_key The pointer to a string specifies the source file itself.
* @param [in] dest_bucket The pointer to a string specifies the bucket to where
*                         the destination file saves.
* @param [in] dest_key The pointer to a string specifies the destination file
*                      itself.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* @retval non-NULL The pointer to a result information object about the copy
*                  operation, or an error message object (see the REMARK
*                  section).
* @retval NULL An application error occurs in copying the file.
*
* @remark The qn_stor_copy() funciton makes a total copy of a file, including
*         the meta and body data.
*
*         No data will be returned if the API succeeds, instead the function
*         returns a JSON object to describe the situation, which contains
*         two error-related fields:
*
*         ```
*             {
*                 "fn-code": <The HTTP code of the response>,
*                 "fn-error": "<The HTTP message of the response>",
*             }
*         ```
*
*         All HTTP codes and corresponding messages list as follow.
*
*         +-------+-------------------------------------------------------+
*         | Code  | Message                                               |
*         +-------+-------------------------------------------------------+
*         | 200   | OK.                                                   |
*         +-------+-------------------------------------------------------+
*         | 400   | Invalid HTTP request.                                 |
*         +-------+-------------------------------------------------------+
*         | 401   | Bad access token (failed in authorization check).     |
*         +-------+-------------------------------------------------------+
*         | 599   | Server failed due to unknown reason and CONTACT US!   |
*         +-------+-------------------------------------------------------+
*         | 612   | Source file doesn't exist.                            |
*         +-------+-------------------------------------------------------+
*         | 614   | Destination file exists.                              |
*         +-------+-------------------------------------------------------+
*         | 631   | Source or destination bucket doesn't exist.           |
*         +-------+-------------------------------------------------------+
*
*         **NOTE**: The caller MUST NOT destroy the result or error object
*                   because the next call to storage core functions will do
*                   that.
*
*         If fails, the function returns a NULL value and the caller can call
*         qn_err_get_message() to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_copy(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_copy_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string op;
    qn_string url;
    qn_string url_tmp;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(mac);
    assert(src_bucket);
    assert(src_key);
    assert(dest_bucket);
    assert(dest_key);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } // if

    // ---- Prepare the copy URL.
    op = qn_stor_make_copy_op(src_bucket, src_key, dest_bucket, dest_key);
    if (!op) return NULL;

    url = qn_cs_sprintf("%.*s/%.*s", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), qn_str_size(op), qn_str_cstr(op));
    qn_str_destroy(op);
    if (!url) return NULL;

    // -- Handle the request to overwrite an existing file.
    if (ext) {
        if (ext->force) {
            url_tmp = qn_cs_sprintf("%s/force/true", url);
            qn_str_destroy(url);
            if (!url_tmp) return NULL;
            url = url_tmp;
        } // if
    } // if

    // ---- Prepare the request and response.
    qn_stor_reset(stor);

    // -- Nothing to post.
    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_for_managing(stor, url, rgn_entry->hostname, mac)) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (stor->obj_body = qn_json_create_object())) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the copy action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);

    if (!ret) return NULL;
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

typedef struct _QN_STOR_MOVE_EXTRA
{
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_move_extra_st;

QN_API qn_stor_move_extra_ptr qn_stor_me_create(void)
{
    qn_stor_move_extra_ptr new_me = calloc(1, sizeof(qn_stor_move_extra_st));
    if (! new_me) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_me;
}

QN_API void qn_stor_me_destroy(qn_stor_move_extra_ptr restrict me)
{
    if (me) {
        free(me);
    } // if
}

QN_API void qn_stor_me_reset(qn_stor_move_extra_ptr restrict me)
{
    memset(me, 0, sizeof(qn_stor_move_extra_st));
}

QN_API void qn_stor_me_set_region_entry(qn_stor_move_extra_ptr restrict me, qn_rgn_entry_ptr restrict entry)
{
    me->rgn_entry = entry;
}

static const qn_string qn_stor_make_move_op(const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key)
{
    qn_string op;
    qn_string encoded_src_uri;
    qn_string encoded_dest_uri;

    encoded_src_uri = qn_misc_encode_uri(src_bucket, src_key);
    if (!encoded_src_uri) return NULL;

    encoded_dest_uri = qn_misc_encode_uri(dest_bucket, dest_key);
    if (!encoded_dest_uri) {
        qn_str_destroy(encoded_src_uri);
        return NULL;
    } // if

    op = qn_cs_sprintf("move/%.*s/%.*s", qn_str_size(encoded_src_uri), qn_str_cstr(encoded_src_uri), qn_str_size(encoded_dest_uri), qn_str_cstr(encoded_dest_uri));
    qn_str_destroy(encoded_dest_uri);
    qn_str_destroy(encoded_src_uri);
    return op;
}

/***************************************************************************//**
* @ingroup Storage-Management
*
* Move a source file to a specified bucket, and/or rename it.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in] src_bucket The pointer to a string specifies the bucket where the
*                        source file resides.
* @param [in] src_key The pointer to a string specifies the source file itself.
* @param [in] dest_bucket The pointer to a string specifies the bucket to where
*                         the source file moves.
* @param [in] dest_key The pointer to a string specifies the final name of the
*                      moved file.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* @retval non-NULL The pointer to a result information object about the move
*                  operation, or an error message object (see the REMARK
*                  section).
* @retval NULL An application error occurs in moving the file.
*
* @remark The qn_stor_move() funciton moves the specified file from the source
*         bucket to the destination bucket, and/or renames it. If there is a
*         file with the same key exists in the destination bucket, it will be
*         overwritten. This behavior is often used to rename an existing file.
*
*         No data will be returned if the API succeeds, instead the function
*         returns a JSON object to describe the situation, which contains
*         two error-related fields:
*
*         ```
*             {
*                 "fn-code": <The HTTP code of the response>,
*                 "fn-error": "<The HTTP message of the response>",
*             }
*         ```
*
*         All HTTP codes and corresponding messages list as follow.
*
*         +-------+-------------------------------------------------------+
*         | Code  | Message                                               |
*         +-------+-------------------------------------------------------+
*         | 200   | OK.                                                   |
*         +-------+-------------------------------------------------------+
*         | 400   | Invalid HTTP request.                                 |
*         +-------+-------------------------------------------------------+
*         | 401   | Bad access token (failed in authorization check).     |
*         +-------+-------------------------------------------------------+
*         | 599   | Server failed due to unknown reason and CONTACT US!   |
*         +-------+-------------------------------------------------------+
*         | 612   | Source file doesn't exist.                            |
*         +-------+-------------------------------------------------------+
*         | 631   | Source or destination bucket doesn't exist.           |
*         +-------+-------------------------------------------------------+
*
*         **NOTE**: The caller MUST NOT destroy the result or error object
*                   because the next call to storage core functions will do
*                   that.
*
*         If fails, the function returns a NULL value and the caller can call
*         qn_err_get_message() to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_move(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_move_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string op;
    qn_string url;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(mac);
    assert(src_bucket);
    assert(src_key);
    assert(dest_bucket);
    assert(dest_key);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } // if

    // ---- Prepare the move URL.
    op = qn_stor_make_move_op(src_bucket, src_key, dest_bucket, dest_key);
    if (!op) return NULL;

    url = qn_cs_sprintf("%.*s/%.*s", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), qn_str_size(op), qn_str_cstr(op));
    qn_str_destroy(op);
    if (!url) return NULL;

    // ---- Prepare the request and response.
    qn_stor_reset(stor);

    // -- Nothing to post.
    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_for_managing(stor, url, rgn_entry->hostname, mac)) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (stor->obj_body = qn_json_create_object())) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the move action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);

    if (!ret) return NULL;
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

typedef struct _QN_STOR_DELETE_EXTRA
{
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_delete_extra_st;

QN_API qn_stor_delete_extra_ptr qn_stor_de_create(void)
{
    qn_stor_delete_extra_ptr new_de = calloc(1, sizeof(qn_stor_delete_extra_st));
    if (! new_de) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_de;
}

QN_API void qn_stor_de_destroy(qn_stor_delete_extra_ptr restrict de)
{
    if (de) {
        free(de);
    } // if
}

QN_API void qn_stor_de_reset(qn_stor_delete_extra_ptr restrict de)
{
    memset(de, 0, sizeof(qn_stor_delete_extra_st));
}

QN_API void qn_stor_de_set_region_entry(qn_stor_delete_extra_ptr restrict de, qn_rgn_entry_ptr restrict entry)
{
    de->rgn_entry = entry;
}

static qn_string qn_stor_make_delete_op(const char * restrict bucket, const char * restrict key)
{
    qn_string op;
    qn_string encoded_uri;

    encoded_uri = qn_misc_encode_uri(bucket, key);
    if (!encoded_uri) return NULL;

    op = qn_cs_sprintf("delete/%.*s", qn_str_size(encoded_uri), qn_str_cstr(encoded_uri));
    qn_str_destroy(encoded_uri);
    return op;
}

/***************************************************************************//**
* @ingroup Storage-Management
*
* Delete a specified file physically.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in] bucket The pointer to a string specifies the bucket where the
*                    file resides.
* @param [in] key The pointer to a string specifies the file itself.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* @retval non-NULL The pointer to a result information object about the delete
*                  operation, or an error message object (see the REMARK
*                  section).
* @retval NULL An application error occurs in deleting the file.
*
* @remark The qn_stor_delete() funciton delets the specified file and after
*         that the file can not be restored or rescued.
*
*         No data will be returned if the API succeeds, instead the function
*         returns a JSON object to describe the situation, which contains
*         two error-related fields:
*
*         ```
*             {
*                 "fn-code": <The HTTP code of the response>,
*                 "fn-error": "<The HTTP message of the response>",
*             }
*         ```
*
*         All HTTP codes and corresponding messages list as follow.
*
*         +-------+-------------------------------------------------------+
*         | Code  | Message                                               |
*         +-------+-------------------------------------------------------+
*         | 200   | OK.                                                   |
*         +-------+-------------------------------------------------------+
*         | 400   | Invalid HTTP request.                                 |
*         +-------+-------------------------------------------------------+
*         | 401   | Bad access token (failed in authorization check).     |
*         +-------+-------------------------------------------------------+
*         | 599   | Server failed due to unknown reason and CONTACT US!   |
*         +-------+-------------------------------------------------------+
*         | 612   | File doesn't exist.                                   |
*         +-------+-------------------------------------------------------+
*         | 631   | Bucket doesn't exist.                                 |
*         +-------+-------------------------------------------------------+
*
*         **NOTE**: The caller MUST NOT destroy the result or error object
*                   because the next call to storage core functions will do
*                   that.
*
*         If fails, the function returns a NULL value and the caller can call
*         qn_err_get_message() to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_delete(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_delete_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string op;
    qn_string url;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(mac);
    assert(bucket);
    assert(key);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } // if

    // ---- Prepare the delete URL.
    op = qn_stor_make_delete_op(bucket, key);
    if (!op) return NULL;

    url = qn_cs_sprintf("%.*s/%.*s", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), qn_str_size(op), qn_str_cstr(op));
    qn_str_destroy(op);
    if (!url) return NULL;

    // ---- Prepare the request and response.
    qn_stor_reset(stor);

    // -- Nothing to post.
    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_for_managing(stor, url, rgn_entry->hostname, mac)) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (stor->obj_body = qn_json_create_object())) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the delete action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);

    if (!ret) return NULL;
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

typedef struct _QN_STOR_CHANGE_MIME_EXTRA
{
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_change_mime_extra_st;

QN_API qn_stor_change_mime_extra_ptr qn_stor_cme_create(void)
{
    qn_stor_change_mime_extra_ptr new_cme = calloc(1, sizeof(qn_stor_change_mime_extra_st));
    if (! new_cme) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_cme;
}

QN_API void qn_stor_cme_destroy(qn_stor_change_mime_extra_ptr restrict cme)
{
    if (cme) {
        free(cme);
    } // if
}

QN_API void qn_stor_cme_reset(qn_stor_change_mime_extra_ptr restrict cme)
{
    memset(cme, 0, sizeof(qn_stor_change_mime_extra_st));
}

QN_API void qn_stor_cme_set_region_entry(qn_stor_change_mime_extra_ptr restrict cme, qn_rgn_entry_ptr restrict entry)
{
    cme->rgn_entry = entry;
}

/***************************************************************************//**
* @ingroup Storage-Management
*
* Change the MIME type of a specified file.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in] bucket The pointer to a string specifies the bucket where the
*                    file resides.
* @param [in] key The pointer to a string specifies the file itself.
* @param [in] mime The pointer to a string specifies the new MIME type.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* @retval non-NULL The pointer to a result information object about the
*                  change-mime operation, or an error message object (see
*                  the REMARK section).
* @retval NULL An application error occurs in changing the type.
*
* @remark The qn_stor_change_mime() funciton changes the MIME type of the
*         specified file to a new one.
*
*         No data will be returned if the API succeeds, instead the function
*         returns a JSON object to describe the situation, which contains
*         two error-related fields:
*
*         ```
*             {
*                 "fn-code": <The HTTP code of the response>,
*                 "fn-error": "<The HTTP message of the response>",
*             }
*         ```
*
*         All HTTP codes and corresponding messages list as follow.
*
*         +-------+-------------------------------------------------------+
*         | Code  | Message                                               |
*         +-------+-------------------------------------------------------+
*         | 200   | OK.                                                   |
*         +-------+-------------------------------------------------------+
*         | 400   | Invalid HTTP request.                                 |
*         +-------+-------------------------------------------------------+
*         | 401   | Bad access token (failed in authorization check).     |
*         +-------+-------------------------------------------------------+
*         | 599   | Server failed due to unknown reason and CONTACT US!   |
*         +-------+-------------------------------------------------------+
*         | 612   | File doesn't exist.                                   |
*         +-------+-------------------------------------------------------+
*         | 631   | Bucket doesn't exist.                                 |
*         +-------+-------------------------------------------------------+
*
*         **NOTE**: The caller MUST NOT destroy the result or error object
*                   because the next call to storage core functions will do
*                   that.
*
*         If fails, the function returns a NULL value and the caller can call
*         qn_err_get_message() to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_change_mime(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, const char * restrict mime, qn_stor_change_mime_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_uri;
    qn_string encoded_mime;
    qn_string url;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(mac);
    assert(bucket);
    assert(key);
    assert(mime);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } // if

    // ---- Prepare the change mime URL.
    encoded_uri = qn_misc_encode_uri(bucket, key);
    if (!encoded_uri) return NULL;

    encoded_mime = qn_cs_encode_base64_urlsafe(mime, strlen(mime));
    if (!encoded_mime) {
        qn_str_destroy(encoded_uri);
        return NULL;
    } // if

    url = qn_cs_sprintf("%.*s/chgm/%.*s/mime/%.*s", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), qn_str_size(encoded_uri), qn_str_cstr(encoded_uri), qn_str_size(encoded_mime), qn_str_cstr(encoded_mime));
    qn_str_destroy(encoded_uri);
    qn_str_destroy(encoded_mime);
    if (!url) return NULL;

    // ---- Prepare the request and response
    qn_stor_reset(stor);

    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_for_managing(stor, url, rgn_entry->hostname, mac)) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (stor->obj_body = qn_json_create_object())) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the change mime action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);

    if (!ret) return NULL;
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

// ----

typedef struct _QN_STOR_FETCH_EXTRA
{
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_fetch_extra_st;

QN_API qn_stor_fetch_extra_ptr qn_stor_fe_create(void)
{
    qn_stor_fetch_extra_ptr new_fe = calloc(1, sizeof(qn_stor_fetch_extra_st));
    if (! new_fe) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_fe;
}

QN_API void qn_stor_fe_destroy(qn_stor_fetch_extra_ptr restrict fe)
{
    if (fe) {
        free(fe);
    } // if
}

QN_API void qn_stor_fe_reset(qn_stor_fetch_extra_ptr restrict fe)
{
    memset(fe, 0, sizeof(qn_stor_fetch_extra_st));
}

QN_API void qn_stor_fe_set_region_entry(qn_stor_fetch_extra_ptr restrict fe, qn_rgn_entry_ptr restrict entry)
{
    fe->rgn_entry = entry;
}

/***************************************************************************//**
* @ingroup Storage-Management
*
* Fetch a file from a third-party origin site and save it to destination bucket.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in] src_url The pointer to a string specifies the source file's URL.
* @param [in] dest_bucket The pointer to a string specifies the bucket to where
*                         the destination file saves.
* @param [in] dest_key The pointer to a string specifies the name of the
*                      destination file.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* @retval non-NULL The pointer to a result information object about the
*                  fetch operation, or an error message object (see the
*                  REMARK section).
* @retval NULL An application error occurs in fetching the file.
*
* @remark The qn_stor_fetch() funciton fetches a file from the specified URL
*         and save it to the destination bucket. It is a good way to do data
*         transfering.
*
*         **NOTE**: The destination file will be overwritten if it exists.
*
*         If succeeds, the function will return a meta information object
*         contains following fields:
*
*         ```
*             {
*                 "fn-code": <The HTTP code of the response>,
*                 "fn-error": "<The HTTP message of the response>",
*                 "fsize": <File's size in bytes>,
*                 "hash": "<File's hash digest generated by Qiniu-ETAG algorithm>",
*                 "mimeType": "<File's MIME type>",
*                 "key": <File's final name>
*             }
*         ```
*
*         The `fn-code` and `fn-error` fields hold the HTTP response's status
*         code and message, respectively. They are always returned by the function
*         if the HTTP response returns successfully, no matter the API's operation
*         succeeds or not.
*
*         Other fields are returned only in the case that the API's operation
*         succeeds.
*
*         All HTTP codes and corresponding messages list as follow.
*
*         +-------+-------------------------------------------------------+
*         | Code  | Message                                               |
*         +-------+-------------------------------------------------------+
*         | 200   | OK.                                                   |
*         +-------+-------------------------------------------------------+
*         | 400   | Invalid HTTP request.                                 |
*         +-------+-------------------------------------------------------+
*         | 401   | Bad access token (failed in authorization check).     |
*         +-------+-------------------------------------------------------+
*         | 404   | The specfied file or resource doesn't exist.          |
*         +-------+-------------------------------------------------------+
*         | 478   | Any HTTP code other than 404 from the origin site     |
*         |       | will turn to this one.                                |
*         +-------+-------------------------------------------------------+
*         | 599   | Server failed due to unknown reason and CONTACT US!   |
*         +-------+-------------------------------------------------------+
*         |       | In the case of other HTTP-codes, check the            |
*         |       | availability of the origin site.                      |
*         +-------+-------------------------------------------------------+
*
*         **NOTE**: The caller MUST NOT destroy the result or error object
*                   because the next call to storage core functions will do
*                   that.
*
*         If fails, the function returns a NULL value and the caller can call
*         qn_err_get_message() to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_fetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_url, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_src_url;
    qn_string encoded_dest_uri;
    qn_string url;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(mac);
    assert(src_url);
    assert(dest_bucket);
    assert(dest_key);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_IO, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_IO, NULL, &rgn_entry);
    } // if

    // ---- Prepare the fetch URL.
    encoded_src_url = qn_cs_encode_base64_urlsafe(src_url, strlen(src_url));
    if (!encoded_src_url) return NULL;

    encoded_dest_uri = qn_misc_encode_uri(dest_bucket, dest_key);
    if (!encoded_dest_uri) {
        qn_str_destroy(encoded_src_url);
        return NULL;
    } // if

    url = qn_cs_sprintf("%.*s/fetch/%.*s/to/%.*s", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), qn_str_size(encoded_src_url), qn_str_cstr(encoded_src_url), qn_str_size(encoded_dest_uri), qn_str_cstr(encoded_dest_uri));
    qn_str_destroy(encoded_src_url);
    qn_str_destroy(encoded_dest_uri);
    if (!url) return NULL;

    // ---- Prepare the request and response.
    qn_stor_reset(stor);

    // -- Nothing to post.
    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_for_managing(stor, url, rgn_entry->hostname, mac)) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (stor->obj_body = qn_json_create_object())) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the fetch action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);

    if (!ret) return NULL;
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

/***************************************************************************//**
* @ingroup Storage-Management
*
* Fetch a file from the binding origin site of a bucket and save it.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in] bucket The pointer to a string specifies the bucket to where the
*                    destination file saves.
* @param [in] key The pointer to a string specifies the name of the source and
*                 destination file.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* @retval non-NULL The pointer to a result object about the prefetch operation,
*                  or an error object (see the REMARK section).
* @retval NULL Failed in prefetching file.
*
* @remark The qn_stor_prefetch() funciton causes the server to fetch a file
*         specified by the key from the binding origin site of the bucket. It
*         is a good way to do file transfering.
*
*         The origin site must be binded to the bucket first via
*         http://portal.qiniu.com before call this function. And the final URL
*         of the source file will be http://<site_domain>/<key>.
*
*         **NOTE**: The destination file will be overwritten if it exists.
*         **NOTE**: The HTTP session will block when the server is fetching the
*                   file. So it may time out if the origin site takes too much
*                   time to return the file.
*
*         No data will be returned if the API succeeds, instead the function
*         returns a JSON object to describe the situation, which contains
*         two error-related fields:
*
*         ```
*             {
*                 "fn-code": <The HTTP code of the response>,
*                 "fn-error": "<The HTTP message of the response>",
*             }
*         ```
*
*         All HTTP codes and corresponding messages list as follow.
*
*         +-------+-------------------------------------------------------+
*         | Code  | Message                                               |
*         +-------+-------------------------------------------------------+
*         | 200   | OK.                                                   |
*         +-------+-------------------------------------------------------+
*         | 400   | Invalid HTTP request.                                 |
*         +-------+-------------------------------------------------------+
*         | 401   | Bad access token (failed in authorization check).     |
*         +-------+-------------------------------------------------------+
*         | 404   | The specfied file or resource doesn't exist.          |
*         +-------+-------------------------------------------------------+
*         | 478   | Any HTTP code other than 404 from the origin site     |
*         |       | will turn to this one.                                |
*         +-------+-------------------------------------------------------+
*         | 599   | Server failed due to unknown reason and CONTACT US!   |
*         +-------+-------------------------------------------------------+
*         |       | In the case of other HTTP-codes, check the            |
*         |       | availability of the origin site.                      |
*         +-------+-------------------------------------------------------+
*
*         **NOTE**: The caller MUST NOT destroy the result or error object
*                   because the next call to storage core functions will do
*                   that.
*
*         If fails, the function returns a NULL value and the caller can call
*         qn_err_get_message() to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_prefetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_dest_uri;
    qn_string url;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(mac);
    assert(dest_bucket);
    assert(dest_key);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_IO, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_IO, NULL, &rgn_entry);
    } // if

    // ---- Prepare the prefetch URL.
    encoded_dest_uri = qn_misc_encode_uri(dest_bucket, dest_key);
    if (!encoded_dest_uri) return NULL;

    url = qn_cs_sprintf("%.s/prefetch/%.s", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), qn_str_size(encoded_dest_uri), qn_str_cstr(encoded_dest_uri));
    qn_str_destroy(encoded_dest_uri);
    if (!url) return NULL;

    // ---- Prepare the request and response.
    qn_stor_reset(stor);

    // -- Nothing to post.
    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_for_managing(stor, url, rgn_entry->hostname, mac)) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (stor->obj_body = qn_json_create_object())) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the prefetch action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);

    if (!ret) return NULL;
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

// ----

typedef struct _QN_STOR_LIST_EXTRA
{
    const char * prefix;
    const char * delimiter;
    const char * marker;
    int limit;

    qn_http_query_ptr qry;
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_list_extra_st;

QN_API qn_stor_list_extra_ptr qn_stor_le_create(void)
{
    qn_stor_list_extra_ptr new_le = malloc(sizeof(qn_stor_list_extra_st));
    if (! new_le) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_le->qry = qn_http_qry_create();
    if (! new_le->qry) {
        free(new_le);
        return NULL;
    } // if

    qn_stor_le_reset(new_le);
    return new_le;
}

QN_API void qn_stor_le_destroy(qn_stor_list_extra_ptr restrict le)
{
    if (le) {
        qn_http_qry_destroy(le->qry);
        free(le);
    } // if
}

QN_API void qn_stor_le_reset(qn_stor_list_extra_ptr restrict le)
{
    le->prefix = NULL;
    le->delimiter = NULL;
    le->marker = NULL;
    le->limit = 1000;
    le->rgn_entry = NULL;
}

QN_API void qn_stor_le_set_prefix(qn_stor_list_extra_ptr restrict le, const char * restrict prefix)
{
    le->prefix = prefix;
}

QN_API void qn_stor_le_set_delimiter(qn_stor_list_extra_ptr restrict le, const char * restrict delimiter)
{
    le->delimiter = delimiter;
}

QN_API void qn_stor_le_set_marker(qn_stor_list_extra_ptr restrict le, const char * restrict marker)
{
    le->marker = marker;
}

QN_API void qn_stor_le_set_limit(qn_stor_list_extra_ptr restrict le, int limit)
{
    le->limit = limit;
}

/***************************************************************************//**
* @ingroup Storage-Management
*
* List all files belong to the specified bucket.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in] bucket The pointer to a string specifies the bucket.
*                    destination file.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* @retval non-NULL The pointer to a result object about the list operation,
*                  or an error object (see the REMARK section).
* @retval NULL Failed in listing files.
*
* @remark The qn_stor_list() funciton lists all or part of files belong to a
*         bucket.
*
*         If succeeds, the function will return the pointer to a result object
*         contains following fields:
*
*         ```
*             {
*                 "fn-code": <The HTTP code of the response>,
*                 "fn-error": "<The HTTP message of the response>",
*
*                 "marker": "<Marker used internally to sustain a query session>",
*
*                 "commonPrefixes": [
*                     "<Common prefix to some files",
*                     ...
*                 ],
*
*                 "items": [
*                     {
*                         "key": "<File's key>",
*                         "putTime": <File's last upload timestamp>,
*                         "hash": "<File's hash digest generated by Qiniu-ETAG algorithm>",
*                         "fsize": <File's size in bytes>,
*                         "mimeType": "<File's MIME type>",
*                         "customer": "<File's user-identified data (if any)>"
*                     },
*                     ...
*                 ]
*             }
*         ```
*
*         The `fn-code` and `fn-error` fields hold the HTTP response's status
*         code and message, respectively. They are always returned by the function
*         if the HTTP response returns successfully, no matter the API's operation
*         succeeds or not.
*
*         **NOTE** : Do NOT touch the `mark` field since it is used internally.
*
*         The `commonPrefixes` field holds every unique and common path prefix to
*         different files. It will be returned only in the case that the `delimiter`
*         option field of the `ext` argument is set to a delimiter string like `/`
*         which is used to delimit the path name and file name parts of the key,
*         and the `prefix` option field unset at the same time. The server collects
*         all path name parts in a JSON object, treats them as common prefixes,
*         and returns with all repeats removed.
*
*         The `items` field holds all records of files, one object for each. It is
*         returned always, even though there are no files at all (and it is an
*         empty array in this case).
*
*         All HTTP codes and corresponding messages list as follow.
*
*         +-------+-------------------------------------------------------+
*         | Code  | Message                                               |
*         +-------+-------------------------------------------------------+
*         | 200   | OK.                                                   |
*         +-------+-------------------------------------------------------+
*         | 400   | Invalid HTTP request.                                 |
*         +-------+-------------------------------------------------------+
*         | 401   | Bad access token (failed in authorization check).     |
*         +-------+-------------------------------------------------------+
*         | 599   | Server failed due to unknown reason and CONTACT US!   |
*         +-------+-------------------------------------------------------+
*         | 631   | Bucket doesn't exist.                                 |
*         +-------+-------------------------------------------------------+
*
*         **NOTE**: The caller MUST NOT destroy the result or error object
*                   because the next call to storage core functions will do
*                   that.
*
*         If fails, the function returns a NULL value and the caller can call
*         qn_err_get_message() to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_list(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, qn_stor_list_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string url;
    qn_string qry_str;
    qn_http_query_ptr qry;
    qn_rgn_entry_ptr rgn_entry;
    int limit = 1000;

    assert(stor);
    assert(mac);
    assert(bucket);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RSF, NULL, &rgn_entry);

        qry = ext->qry;

        limit = (0 < ext->limit && ext->limit <= 1000) ? ext->limit : 1000;

        if (ext->delimiter && strlen(ext->delimiter) && ! qn_http_qry_set_string(qry, "delimiter", ext->delimiter)) return NULL;
        if (ext->prefix && strlen(ext->prefix) && ! qn_http_qry_set_string(qry, "prefix", ext->prefix)) return NULL;
        if (ext->marker && strlen(ext->marker) && ! qn_http_qry_set_string(qry, "marker", ext->marker)) return NULL;
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RSF, NULL, &rgn_entry);

        qry = qn_http_qry_create();
        if (! qry) return NULL;
    } // if

    if (! qn_http_qry_set_string(qry, "bucket", bucket)) {
        if (! ext) qn_http_qry_destroy(qry);
        return NULL;
    } // if

    if (! qn_http_qry_set_integer(qry, "limit", limit)) {
        if (! ext) qn_http_qry_destroy(qry);
        return NULL;
    } // if

    qry_str = qn_http_qry_to_string(qry);
    if (! ext) qn_http_qry_destroy(qry);
    if (! qry_str) return NULL;

    // ---- Prepare the copy URL.
    url = qn_cs_sprintf("%.*s/list?%.*s", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), qn_str_size(qry_str), qn_str_cstr(qry_str));
    qn_str_destroy(qry_str);
    if (! url) return NULL;

    // ---- Prepare the request and response.
    qn_stor_reset(stor);

    qn_http_req_set_body_data(stor->req, "", 0);

    if (! qn_stor_prepare_for_managing(stor, url, rgn_entry->hostname, mac)) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (stor->obj_body = qn_json_create_object())) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) {
        qn_str_destroy(url);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the list action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    if (! ret) return NULL;

    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (! qn_json_rename(stor->obj_body, "error", "fn-error") && ! qn_err_is_no_such_entry()) return NULL;
    return stor->obj_body;
}

// ----

typedef struct _QN_STOR_BATCH
{
    qn_string * ops;
    int cnt;
    int cap;
} qn_stor_batch;

QN_API qn_stor_batch_ptr qn_stor_bt_create(void)
{
    qn_stor_batch_ptr new_bt = calloc(1, sizeof(qn_stor_batch));
    if (!new_bt) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_bt->cap = 4;
    new_bt->ops = calloc(new_bt->cap, sizeof(qn_string));
    if (!new_bt->ops) {
        free(new_bt);
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_bt;
}

QN_API void qn_stor_bt_destroy(qn_stor_batch_ptr restrict bt)
{
    if (bt) {
        qn_stor_bt_reset(bt);
        free(bt->ops);
        free(bt);
    } // if
}

QN_API void qn_stor_bt_reset(qn_stor_batch_ptr restrict bt)
{
    while (bt->cnt > 0) qn_str_destroy(bt->ops[--bt->cnt]);
}

static qn_bool qn_stor_bt_augment(qn_stor_batch_ptr restrict bt)
{
    int new_cap = bt->cnt + (bt->cnt >> 1); // 1.5 times
    qn_string * new_ops = calloc(new_cap, sizeof(qn_string));
    if (!new_ops) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    memcpy(new_ops, bt->ops, bt->cnt * sizeof(qn_string));
    free(bt->ops);
    bt->ops = new_ops;
    bt->cap = new_cap;
    return qn_true;
}

static qn_bool qn_stor_bt_add_op(qn_stor_batch_ptr restrict bt, const qn_string restrict op)
{
    qn_string encoded_op;
    qn_string entry;

    if (bt->cnt == bt->cap && !qn_stor_bt_augment(bt)) return qn_false;

    encoded_op = qn_cs_percent_encode(qn_str_cstr(op), qn_str_size(op));
    if (!encoded_op) return qn_false;

    entry = qn_cs_sprintf("op=%.*s", qn_str_size(encoded_op), qn_str_cstr(encoded_op));
    qn_str_destroy(encoded_op);
    if (!entry) return qn_false;

    bt->ops[bt->cnt++] = entry;
    return qn_true;
}

QN_API qn_bool qn_stor_bt_add_stat_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key)
{
    qn_bool ret;
    qn_string op;

    op = qn_stor_make_stat_op(bucket, key);
    if (!op) return qn_false;

    ret = qn_stor_bt_add_op(bt, op);
    qn_str_destroy(op);
    return ret;
}

QN_API qn_bool qn_stor_bt_add_copy_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key)
{
    qn_bool ret;
    qn_string op;

    op = qn_stor_make_copy_op(src_bucket, src_key, dest_bucket, dest_key);
    if (!op) return qn_false;

    ret = qn_stor_bt_add_op(bt, op);
    qn_str_destroy(op);
    return ret;
}

QN_API qn_bool qn_stor_bt_add_move_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key)
{
    qn_bool ret;
    qn_string op;

    op = qn_stor_make_move_op(src_bucket, src_key, dest_bucket, dest_key);
    if (!op) return qn_false;

    ret = qn_stor_bt_add_op(bt, op);
    qn_str_destroy(op);
    return ret;
}

QN_API qn_bool qn_stor_bt_add_delete_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key)
{
    qn_bool ret;
    qn_string op;

    op = qn_stor_make_delete_op(bucket, key);
    if (!op) return qn_false;

    ret = qn_stor_bt_add_op(bt, op);
    qn_str_destroy(op);
    return ret;
}

typedef struct _QN_STOR_BATCH_EXTRA
{
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_batch_extra_st;

QN_API qn_stor_batch_extra_ptr qn_stor_be_create(void)
{
    qn_stor_batch_extra_ptr new_be = calloc(1, sizeof(qn_stor_batch_extra_st));
    if (! new_be) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_be;
}

QN_API void qn_stor_be_destroy(qn_stor_batch_extra_ptr restrict be)
{
    if (be) {
        free(be);
    } // if
}

QN_API void qn_stor_be_reset(qn_stor_batch_extra_ptr restrict be)
{
    memset(be, 0, sizeof(qn_stor_batch_extra_st));
}

QN_API void qn_stor_be_set_region_entry(qn_stor_batch_extra_ptr restrict be, qn_rgn_entry_ptr restrict entry)
{
    be->rgn_entry = entry;
}

QN_API qn_json_object_ptr qn_stor_execute_batch_opertions(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const qn_stor_batch_ptr restrict bt, qn_stor_batch_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string body;
    qn_string url;
    qn_json_object_ptr fake_obj_body;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(mac);
    assert(bt);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
    } // if

    // ---- Prepare the batch URL.
    body = qn_str_join_list("&", bt->ops, bt->cnt);
    if (!body) return NULL;

    url = qn_cs_sprintf("%.*s/batch", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url));
    if (!url) {
        qn_str_destroy(body);
        return NULL;
    } // if

    // ---- Prepare the request and response.
    qn_stor_reset(stor);

    qn_http_req_set_body_data(stor->req, qn_str_cstr(body), qn_str_size(body));

    if (!qn_stor_prepare_for_managing(stor, url, rgn_entry->hostname, mac)) {
        qn_str_destroy(url);
        qn_str_destroy(body);
        return NULL;
    } // if

    // Use a fake object to match the return type of all storage main functions.
    if (! (fake_obj_body = qn_json_create_object())) {
        qn_str_destroy(url);
        qn_str_destroy(body);
        return NULL;
    } // if

    if (! (qn_json_set_integer(fake_obj_body, "fn-code", 0))) {
        qn_str_destroy(url);
        qn_str_destroy(body);
        return NULL;
    } // if

    if (! (qn_json_set_string(fake_obj_body, "fn-error", "OK"))) {
        qn_str_destroy(url);
        qn_str_destroy(body);
        return NULL;
    } // if

    if (! (stor->arr_body = qn_json_create_and_set_array(fake_obj_body, "items"))) {
        qn_str_destroy(url);
        qn_str_destroy(body);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, &stor->arr_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the batch action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    qn_str_destroy(body);
    stor->arr_body = NULL; // Keep from destroying the array twice.

    if (!ret) return NULL;
    if (stor->obj_body) {
        // Get an object rather than an array.
        // TODO: Trace the change of return value to this API and make the corresponding fix.
        qn_json_destroy_object(fake_obj_body);
    } else {
        stor->obj_body = fake_obj_body;
    } // if

    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

// ---- Definition of Put Policy ----

QN_API qn_json_object_ptr qn_stor_pp_create(const char * restrict bucket, const char * restrict key, qn_uint32 deadline)
{
    qn_json_object_ptr pp = qn_json_create_object();
    if (pp) {
        if (!qn_stor_pp_set_scope(pp, bucket, key)) return NULL;
        if (!qn_stor_pp_set_deadline(pp, deadline)) return NULL;
    } // if
    return pp;
}

QN_API void qn_stor_pp_destroy(qn_json_object_ptr restrict pp)
{
    qn_json_destroy_object(pp);
}

QN_API qn_bool qn_stor_pp_set_scope(qn_json_object_ptr restrict pp, const char * restrict bucket, const char * restrict key)
{
    qn_bool ret;
    qn_string scope;
    if (key) {
        scope = qn_cs_sprintf("%s:%s", bucket, key);
        if (!scope) return qn_false;
        ret = qn_json_set_string(pp, "scope", qn_str_cstr(scope));
        qn_str_destroy(scope);
    } else {
        ret = qn_json_set_string(pp, "scope", bucket);
    } // if
    return ret;
}

QN_API qn_bool qn_stor_pp_set_deadline(qn_json_object_ptr restrict pp, qn_uint32 deadline)
{
    return qn_json_set_integer(pp, "deadline", deadline);
}

QN_API qn_bool qn_stor_pp_dont_overwrite(qn_json_object_ptr restrict pp)
{
    return qn_json_set_integer(pp, "insertOnly", 1);
}

QN_API qn_bool qn_stor_pp_return_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict body)
{
    if (!qn_json_set_string(pp, "returnUrl", url)) return qn_false;
    if (body) return qn_json_set_string(pp, "returnBody", body);
    return qn_true;
}

QN_API qn_bool qn_stor_pp_return_to_client(qn_json_object_ptr restrict pp, const char * restrict body)
{
    return qn_json_set_string(pp, "returnBody", body);
}

QN_API qn_bool qn_stor_pp_callback_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict host_name)
{
    if (!qn_json_set_string(pp, "callbackUrl", url)) return qn_false;
    if (host_name) return qn_json_set_string(pp, "callbackHost", host_name);
    return qn_true;
}

QN_API qn_bool qn_stor_pp_callback_with_body(qn_json_object_ptr restrict pp, const char * restrict body, const char * restrict mime_type)
{
    if (!qn_json_set_string(pp, "callbackBody", body)) return qn_false;
    if (mime_type) return qn_json_set_string(pp, "callbackBodyType", mime_type);
    return qn_true;
}

QN_API qn_bool qn_stor_pp_pfop_set_commands(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char * restrict cmd1, const char * restrict cmd2, ...)
{
    va_list ap;
    qn_bool ret;
    qn_string ops;

    if (!cmd2) {
        // Only one command passed.
        ret = qn_json_set_string(pp, "persistentOps", cmd1);
    } else {
        va_start(ap, cmd2);
        ops = qn_cs_join_va(";", cmd1, cmd2, ap);
        va_end(ap);
        if (!ops) return qn_false;
        ret = qn_json_set_string(pp, "persistentOps", ops);
        qn_str_destroy(ops);
    } // if

    if (ret && pipeline && strlen(pipeline) > 0) ret = qn_json_set_string(pp, "persistentPipeline", pipeline);
    return ret;
}

QN_API qn_bool qn_stor_pp_pfop_set_command_list(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char ** restrict cmds, int cmd_count)
{
    qn_bool ret = qn_false;
    qn_string ops = NULL;

    if (cmd_count == 1) {
        // Only one command passed.
        ret = qn_json_set_string(pp, "persistentOps", cmds[0]);
    } else {
        ops = qn_cs_join_list(";", cmds, cmd_count);
        if (!ops) return qn_false;
        ret = qn_json_set_string(pp, "persistentOps", ops);
        qn_str_destroy(ops);
    } // if
    
    if (ret && pipeline && strlen(pipeline) > 0) ret = qn_json_set_string(pp, "persistentPipeline", pipeline);
    return ret;
}

QN_API qn_bool qn_stor_pp_pfop_notify_to_server(qn_json_object_ptr restrict pp, const char * restrict url)
{
    return qn_json_set_string(pp, "persistentNotifyUrl", url);
}

QN_API qn_bool qn_stor_pp_mime_enable_auto_detecting(qn_json_object_ptr restrict pp)
{
    return qn_json_set_integer(pp, "detectMime", 1);
}

QN_API qn_bool qn_stor_pp_mime_allow(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string mime_str = NULL;

    if (!mime2) return qn_json_set_string(pp, "mimeLimit", mime1); // Only one mime passed.

    va_start(ap, mime2);
    mime_str = qn_cs_join_va(";", mime1, mime2, ap);
    va_end(ap);
    if (!mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", mime_str);
    qn_str_destroy(mime_str);
    return ret;
}

QN_API qn_bool qn_stor_pp_mime_allow_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, int mime_count)
{
    qn_bool ret = qn_false;
    qn_string mime_str = NULL;
    
    if (mime_count == 1) return qn_json_set_string(pp, "mimeLimit", mime_list[0]);

    mime_str = qn_cs_join_list(";", mime_list, mime_count);
    if (!mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", mime_str);
    qn_str_destroy(mime_str);
    return ret;
}

QN_API qn_bool qn_stor_pp_mime_deny(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...)
{
    va_list ap;
    qn_bool ret;
    qn_string deny_mime_str;
    qn_string mime_str;

    if (!mime2) {
        deny_mime_str = qn_cs_sprintf("!%s", mime1); // Only one mime passed.
    } else {
        va_start(ap, mime2);
        mime_str = qn_cs_join_va(";", mime1, mime2, ap);
        va_end(ap);
        if (!mime_str) return qn_false;
        deny_mime_str = qn_cs_sprintf("!%s", mime_str);
        qn_str_destroy(mime_str);
        if (!deny_mime_str) return qn_false;
    } // if

    ret = qn_json_set_string(pp, "mimeLimit", deny_mime_str);
    qn_str_destroy(deny_mime_str);
    return ret;
}

QN_API qn_bool qn_stor_pp_mime_deny_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, int mime_count)
{
    qn_bool ret;
    qn_string deny_mime_str;
    qn_string mime_str;
    
    if (mime_count == 1) { 
        deny_mime_str = qn_cs_sprintf("!%s", mime_list[0]); // Only one mime passed.
    } else {
        mime_str = qn_cs_join_list(";", mime_list, mime_count);
        if (!mime_str) return qn_false;
        deny_mime_str = qn_cs_sprintf("!%s", mime_str);
        qn_str_destroy(mime_str);
    } // if
    if (!deny_mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", deny_mime_str);
    qn_str_destroy(deny_mime_str);
    return ret;
}

QN_API qn_bool qn_stor_pp_fsize_set_minimum(qn_json_object_ptr restrict pp, qn_uint32 min_size)
{
    return qn_json_set_integer(pp, "fsizeMin", min_size);
}

QN_API qn_bool qn_stor_pp_fsize_set_maximum(qn_json_object_ptr restrict pp, qn_uint32 max_size)
{
    return qn_json_set_integer(pp, "fsizeLimit", max_size);
}

QN_API qn_bool qn_stor_pp_key_enable_fetching_from_callback_response(qn_json_object_ptr restrict pp)
{
    return qn_json_set_integer(pp, "callbackFetchKey", 1);
}

QN_API qn_bool qn_stor_pp_key_make_from_template(qn_json_object_ptr restrict pp, const char * restrict key_template)
{
    return qn_json_set_string(pp, "saveKey", key_template);
}

QN_API qn_bool qn_stor_pp_auto_delete_after_days(qn_json_object_ptr restrict pp, qn_uint32 days)
{
    return qn_json_set_integer(pp, "deleteAfterDays", days);
}

QN_API qn_bool qn_stor_pp_upload_message(qn_json_object_ptr restrict pp, const char * restrict msg_queue, const char * restrict msg_body, const char * restrict msg_mime_type)
{
    if (! qn_json_set_string(pp, "notifyQueue", msg_queue)) return qn_false;
    if (! qn_json_set_string(pp, "notifyMessage", msg_body)) return qn_false;
    if (msg_mime_type) return qn_json_set_string(pp, "notifyMessageType", msg_mime_type);
    return qn_true;
}

QN_API qn_string qn_stor_pp_to_uptoken(qn_json_object_ptr restrict pp, qn_mac_ptr restrict mac)
{
    qn_string uptoken;
    qn_string str = qn_json_object_to_string(pp);

    if (!str) return NULL;

    uptoken = qn_mac_make_uptoken(mac, qn_str_cstr(str), qn_str_size(str));
    qn_str_destroy(str);
    return uptoken;
}

// ---- Definition of Upload ----

typedef struct _QN_STOR_UPLOAD_READER
{
    qn_stor_upload_extra_ptr ext;
} qn_stor_put_reader_st, *qn_stor_put_reader_ptr;

typedef struct _QN_STOR_UPLOAD_EXTRA
{
    const char * final_key;
    const char * crc32;
    const char * accept_type;

    qn_rgn_entry_ptr rgn_entry;

    qn_fsize fsize;
    qn_io_reader_itf rdr;
} qn_stor_upload_extra_st;

QN_API qn_stor_upload_extra_ptr qn_stor_ue_create(void)
{
    qn_stor_upload_extra_ptr new_pe = calloc(1, sizeof(qn_stor_upload_extra_st));
    if (! new_pe) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_pe;
}

QN_API void qn_stor_ue_destroy(qn_stor_upload_extra_ptr restrict ue)
{
    if (ue) {
        free(ue);
    } // if
}

QN_API void qn_stor_ue_reset(qn_stor_upload_extra_ptr restrict ue)
{
    memset(ue, 0, sizeof(qn_stor_upload_extra_st));
}

QN_API void qn_stor_ue_set_final_key(qn_stor_upload_extra_ptr restrict ue, const char * restrict final_key)
{
    ue->final_key = final_key;
}

QN_API extern void qn_stor_ue_set_local_crc32(qn_stor_upload_extra_ptr restrict ue, const char * restrict crc32)
{
    ue->crc32 = crc32;
}

QN_API extern void qn_stor_ue_set_accept_type(qn_stor_upload_extra_ptr restrict ue, const char * restrict accept_type)
{
    ue->accept_type = accept_type;
}

QN_API extern void qn_stor_ue_set_region_entry(qn_stor_upload_extra_ptr restrict ue, qn_rgn_entry_ptr restrict entry)
{
    ue->rgn_entry = entry;
}

static qn_bool qn_stor_prepare_for_upload(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_upload_extra_ptr restrict ext)
{
    qn_http_form_ptr form;

    // ---- Prepare request and response
    qn_stor_reset(stor);

    // ----
    if (!qn_stor_prepare_common_request_headers(stor)) return qn_false;

    // ----
    if (! (form = qn_http_req_prepare_form(stor->req))) return qn_false;

    // **NOTE** : The uptoken MUST be the first form item.
    if (!qn_http_form_add_string(form, "token", uptoken, strlen(uptoken))) return qn_false;
    if (ext->final_key && !qn_http_form_add_string(form, "key", ext->final_key, strlen(ext->final_key))) return qn_false;
    if (ext->crc32 && !qn_http_form_add_string(form, "crc32", ext->crc32, strlen(ext->crc32))) return qn_false;
    if (ext->accept_type && !qn_http_form_add_string(form, "accept", ext->accept_type, strlen(ext->accept_type))) return qn_false;

    // TODO: User defined variabales.

    // ----
    if (! (stor->obj_body = qn_json_create_object())) return NULL;
    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) return NULL;
    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) return NULL;

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    return qn_true;
}

/***************************************************************************//**
* @ingroup Storage-Management
*
* Upload a file to the specified bucket in one HTTP round-trip.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in] fname The pointer to a string specifies the path name of the
*                   local file to be uploaded.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* @retval non-NULL The pointer to a result information object about the file,
*                  or an error message object (see the REMARK section).
* @retval NULL An application error occurs in uploading the file.
*
* @remark The qn_stor_upload_file() uploads a local file in one HTTP round-trip
*         to put it into the destination bucket. It's the simplest way to
*         upload a file.
*
*         If succeeds, the function returns a result information object
*         contains a few of fields describing the meta data about the file,
*         accompanied by some option ones describing other information like
*         data-transform job ID. So it has a basic structure like the
*         following one:
*
*         ```
*             {
*                 "fn-code": <The HTTP code of the response>,
*                 "fn-error": "<The HTTP message of the response>",
*
*                 "fsize": <File's size in bytes>,
*                 "hash": "<File's hash digest generated by Qiniu-ETAG algorithm>",
*                 "mimeType": "<File's MIME type>",
*                 "putTime": <File's last upload timestamp>,
*
*                 "persistentId": <Data tranform job ID>
*             }
*         ```
*
*         The `fn-code` and `fn-error` fields hold the HTTP response's status
*         code and message, respectively. They are always returned by the function
*         if the HTTP response returns successfully, no matter the API's operation
*         succeeds or not.
*
*         Other fields are returned only in the case that the API's operation
*         succeeds.
*
*         All HTTP codes and corresponding messages list as follow.
*
*         +-------+-------------------------------------------------------+
*         | Code  | Message                                               |
*         +-------+-------------------------------------------------------+
*         | 200   | OK.                                                   |
*         +-------+-------------------------------------------------------+
*         | 400   | Invalid HTTP request.                                 |
*         +-------+-------------------------------------------------------+
*         | 401   | Bad access token (failed in authorization check).     |
*         +-------+-------------------------------------------------------+
*         | 599   | Server failed due to unknown reason and CONTACT US!   |
*         +-------+-------------------------------------------------------+
*         | 614   | File exists.                                          |
*         +-------+-------------------------------------------------------+
*         | 631   | Bucket doesn't exist.                                 |
*         +-------+-------------------------------------------------------+
*
*         **NOTE**: The caller MUST NOT destroy the result or error object
*                   because the next call to storage core functions will do
*                   that.
*
*         If fails, the function returns a NULL value and the caller can call
*         qn_err_get_message() to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_upload_file(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict fname, qn_stor_upload_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_fl_info_ptr fi;
    qn_http_form_ptr form;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(uptoken);

    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (!qn_stor_prepare_for_upload(stor, uptoken, ext)) return NULL;

    // ----
    form = qn_http_req_get_form(stor->req);

    fi = qn_fl_info_stat(fname);
    if (!fi) return NULL;

    ret = qn_http_form_add_file(form, "file", qn_str_cstr(qn_fl_info_fname(fi)), NULL, qn_fl_info_fsize(fi));
    qn_fl_info_destroy(fi);
    if (!ret) return NULL;

    // ----
    if (rgn_entry->hostname && !qn_http_req_set_header(stor->req, "Host", qn_str_cstr(rgn_entry->hostname))) return NULL;

    // ----
    ret = qn_http_conn_post(stor->conn, qn_str_cstr(rgn_entry->base_url), stor->req, stor->resp);
    if (!ret) return NULL;

    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

QN_API qn_json_object_ptr qn_stor_upload_buffer(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict buf, int buf_size, qn_stor_upload_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_http_form_ptr form;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(uptoken);
    assert(buf);

    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (!qn_stor_prepare_for_upload(stor, uptoken, ext)) return NULL;

    form = qn_http_req_get_form(stor->req);

    if (!qn_http_form_add_buffer(form, "file", "<null>", buf, buf_size)) return NULL;

    // ----
    if (rgn_entry->hostname && !qn_http_req_set_header(stor->req, "Host", qn_str_cstr(rgn_entry->hostname))) return NULL;
    ret = qn_http_conn_post(stor->conn, qn_str_cstr(rgn_entry->base_url), stor->req, stor->resp);
    if (!ret) return NULL;

    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

static size_t qn_stor_upload_callback_fn(void * user_data, char * buf, size_t size)
{
    qn_io_reader_itf rdr = (qn_io_reader_itf) user_data;
    ssize_t ret;

    ret = qn_io_read(rdr, buf, size);
    if (ret < 0) return CURL_READFUNC_ABORT;
    return ret;
}

QN_API qn_json_object_ptr qn_stor_upload(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict rdr, qn_stor_upload_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(uptoken);
    assert(rdr);

    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (! qn_stor_prepare_for_upload(stor, uptoken, ext)) return NULL;

    ret = qn_http_form_add_file_reader(qn_http_req_get_form(stor->req), "file", qn_str_cstr(qn_io_name(rdr)), NULL, qn_io_size(rdr), stor->req);
    if (! ret) return NULL;

    qn_http_req_set_body_reader(stor->req, rdr, qn_stor_upload_callback_fn, qn_io_size(rdr));

    // ----
    if (rgn_entry->hostname && !qn_http_req_set_header(stor->req, "Host", qn_str_cstr(rgn_entry->hostname))) return NULL;
    ret = qn_http_conn_post(stor->conn, qn_str_cstr(rgn_entry->base_url), stor->req, stor->resp);
    if (! ret) return NULL;

    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (! qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

// ----

typedef struct _QN_STOR_RESUMABLE_PUT_SESSION
{
    qn_fsize fsize;
    qn_json_array_ptr blk_info_list;
} qn_stor_rput_session;

#define QN_STOR_RPUT_BLOCK_MAX_SIZE (1L << 22)
#define QN_STOR_RPUT_CHUNK_DEFAULT_SIZE (1024 * 256)

QN_API qn_stor_rput_session_ptr qn_stor_rs_create(qn_fsize fsize)
{
    int i;
    int blk_count;
    int blk_offset;
    int last_blk_size;
    qn_json_object_ptr blk_info;
    qn_stor_rput_session_ptr new_ss = calloc(1, sizeof(qn_stor_rput_session));
    if (!new_ss) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_ss->blk_info_list = qn_json_create_array();
    if (!new_ss->blk_info_list) {
        free(new_ss);
        return NULL;
    } // if
    new_ss->fsize = fsize;

    blk_offset = 0;
    last_blk_size = fsize % QN_STOR_RPUT_BLOCK_MAX_SIZE;
    blk_count = (fsize / QN_STOR_RPUT_BLOCK_MAX_SIZE);
    for (i = 0; i < blk_count; i += 1) {
        if (! (blk_info = qn_json_create_and_push_object(new_ss->blk_info_list))) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
        if (!qn_json_set_integer(blk_info, "bsize", QN_STOR_RPUT_BLOCK_MAX_SIZE)) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
        if (!qn_json_set_integer(blk_info, "boffset", blk_offset)) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
        blk_offset += QN_STOR_RPUT_BLOCK_MAX_SIZE;
    } // for
    if (last_blk_size > 0) {
        if (! (blk_info = qn_json_create_and_push_object(new_ss->blk_info_list))) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
        if (!qn_json_set_integer(blk_info, "bsize", last_blk_size)) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
        if (!qn_json_set_integer(blk_info, "boffset", blk_offset)) {
            qn_json_destroy_array(new_ss->blk_info_list);
            free(new_ss);
            return NULL;
        } // if
    } // if
    return new_ss;
}

QN_API void qn_stor_rs_destroy(qn_stor_rput_session_ptr restrict ss)
{
    if (ss) {
        qn_json_destroy_array(ss->blk_info_list);
        free(ss);
    } // if
}

QN_API qn_stor_rput_session_ptr qn_stor_rs_from_string(const char * restrict str, size_t str_size)
{
    qn_bool ret;
    size_t ret_size;
    qn_json_parser_ptr prs;
    qn_stor_rput_session_ptr new_ss = calloc(1, sizeof(qn_stor_rput_session));

    if (!new_ss) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    prs = qn_json_prs_create();
    if (!prs) {
        free(new_ss);
        return NULL;
    } // if

    if (str_size <= 0) str_size = strlen(str);
    ret_size = str_size;

    ret = qn_json_prs_parse_array(prs, str, &ret_size, &new_ss->blk_info_list);
    qn_json_prs_destroy(prs);
    if (!ret) {
        free(new_ss);
        return NULL;
    } // if
    return new_ss;
}

QN_API qn_string qn_stor_rs_to_string(const qn_stor_rput_session_ptr restrict ss)
{
    return qn_json_array_to_string(ss->blk_info_list);
}

QN_API int qn_stor_rs_block_count(const qn_stor_rput_session_ptr restrict ss)
{
    return qn_json_size_array(ss->blk_info_list);
}

QN_API int qn_stor_rs_block_size(const qn_stor_rput_session_ptr restrict ss, int n)
{
    qn_json_object_ptr blk_info = qn_json_pick_object(ss->blk_info_list, n, NULL);
    if (!blk_info) return 0;
    return qn_json_get_integer(blk_info, "bsize", 0);
}

QN_API qn_json_object_ptr qn_stor_rs_block_info(const qn_stor_rput_session_ptr restrict ss, int n)
{
    return qn_json_pick_object(ss->blk_info_list, n, NULL);
}

QN_API qn_bool qn_stor_rs_is_putting_block_done(const qn_stor_rput_session_ptr restrict ss, int n)
{
    qn_json_object_ptr blk_info = qn_json_pick_object(ss->blk_info_list, n, NULL);
    if (!blk_info) return qn_false;
    return (qn_json_get_integer(blk_info, "offset", 0) == qn_json_get_integer(blk_info, "bsize", 0));
}

// ----

typedef struct _QN_STOR_RESUMABLE_PUT_EXTRA
{
    unsigned int chk_size:23;

    const char * final_key;

    qn_fsize fsize;
    qn_io_reader_itf rdr;

    qn_rgn_entry_ptr rgn_entry;
} qn_stor_rput_extra_st;

QN_API qn_stor_rput_extra_ptr qn_stor_rpe_create(void)
{
    qn_stor_rput_extra_ptr new_rpe = malloc(sizeof(qn_stor_rput_extra_st));
    if (! new_rpe) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    qn_stor_rpe_reset(new_rpe);
    return new_rpe;
}

QN_API void qn_stor_rpe_destroy(qn_stor_rput_extra_ptr restrict rpe)
{
    if (rpe) {
        free(rpe);
    } // if
}

QN_API void qn_stor_rpe_reset(qn_stor_rput_extra_ptr restrict rpe)
{
    memset(rpe, 0, sizeof(qn_stor_rput_extra_st));
    rpe->chk_size = QN_STOR_RPUT_CHUNK_DEFAULT_SIZE;
}

QN_API void qn_stor_rpe_set_chunk_size(qn_stor_rput_extra_ptr restrict rpe, int chk_size)
{
    if (0 < chk_size && chk_size <= QN_STOR_RPUT_BLOCK_MAX_SIZE) rpe->chk_size = chk_size;
}

QN_API void qn_stor_rpe_set_final_key(qn_stor_rput_extra_ptr restrict rpe, const char * restrict key)
{
    rpe->final_key = key;
}

QN_API void qn_stor_rpe_set_region_entry(qn_stor_rput_extra_ptr restrict rpe, qn_rgn_entry_ptr restrict entry)
{
    rpe->rgn_entry = entry;
}

QN_API void qn_stor_rpe_set_source_reader(qn_stor_rput_extra_ptr restrict rpe, qn_io_reader_itf restrict rdr, qn_fsize fsize)
{
    rpe->rdr = rdr;
    rpe->fsize = fsize;
}

typedef struct _QN_STOR_RESUMABLE_PUT_READER
{
    qn_io_reader_itf rdr;
    qn_stor_rput_extra_ptr ext;
} qn_stor_rput_reader, *qn_stor_rput_reader_ptr;

static size_t qn_stor_rp_chunk_body_reader_callback(void * user_data, char * buf, size_t size)
{
    size_t ret;
    qn_stor_rput_reader_ptr chk_rdr = (qn_stor_rput_reader_ptr) user_data;
    ret = qn_io_read(chk_rdr->rdr, buf, size);
    return ret;
}

static qn_json_object_ptr qn_stor_rp_put_chunk_in_one_piece(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_json_object_ptr restrict blk_info, qn_io_reader_itf restrict rdr, int chk_size, const qn_string restrict url, const qn_rgn_entry_ptr rgn_entry, qn_stor_rput_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string auth_header;
    qn_string content_length;
    qn_string ctx;
    qn_string checksum;
    qn_string host;
    qn_integer crc32;
    qn_integer offset;
    qn_stor_rput_reader chk_rdr;

    // ---- Reset the stor object.
    qn_stor_reset(stor);

    // ---- Set all the HTTP request headers.
    // -- Set the `Authorization` header.
    auth_header = qn_cs_sprintf("UpToken %s", uptoken);
    if (!auth_header) return NULL;

    ret = qn_http_req_set_header(stor->req, "Authorization", auth_header);
    qn_str_destroy(auth_header);
    if (!ret) return NULL;

    // -- Set the common headers.
    if (!qn_stor_prepare_common_request_headers(stor)) return NULL;

    // -- Set the `Host` header if a host domain exists.
    if (rgn_entry->hostname && !qn_http_req_set_header(stor->req, "Host", qn_str_cstr(rgn_entry->hostname))) return NULL;

    //  -- Set the `Content-Type` and `Content-Length` headers.
    if (!qn_http_req_set_header(stor->req, "Content-Type", "application/octet-stream")) return NULL;

    content_length = qn_cs_sprintf("%d", chk_size);
    if (!content_length) return NULL;

    ret = qn_http_req_set_header(stor->req, "Content-Length", qn_str_cstr(content_length));
    qn_str_destroy(content_length);
    if (!ret) return NULL;

    // ---- Prepare the section reader.
    offset = qn_json_get_integer(blk_info, "boffset", -QN_STOR_RPUT_BLOCK_MAX_SIZE) + qn_json_get_integer(blk_info, "offset", 0);
    if (offset < 0) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if

    memset(&chk_rdr, 0, sizeof(qn_stor_rput_reader));
    chk_rdr.rdr = qn_io_section(rdr, offset, chk_size);
    chk_rdr.ext = ext;

    if (!chk_rdr.rdr) return NULL;

    qn_http_req_set_body_reader(stor->req, &chk_rdr, qn_stor_rp_chunk_body_reader_callback, chk_size);

    // ---- Prepare the JSON body writer.
    if (! (stor->obj_body = qn_json_create_object())) {
        qn_io_close(chk_rdr.rdr);
        return NULL;
    } // if

    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) {
        qn_io_close(chk_rdr.rdr);
        return NULL;
    } // if

    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) {
        qn_io_close(chk_rdr.rdr);
        return NULL;
    } // if

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the put action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_io_close(chk_rdr.rdr);

    if (!ret) return NULL;
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error") && !qn_err_is_no_such_entry()) return NULL;

    if (qn_http_resp_get_code(stor->resp) != 200) return stor->obj_body;

    // ---- Get and set the returned block information back into the blk_info object.
    ctx = qn_json_get_string(stor->obj_body, "ctx", NULL);
    checksum = qn_json_get_string(stor->obj_body, "checksum", NULL);
    host = qn_json_get_string(stor->obj_body, "host", NULL);
    crc32 = qn_json_get_integer(stor->obj_body, "crc32", -1);
    offset = qn_json_get_integer(stor->obj_body, "offset", -1);

    if (!ctx || !checksum || !host || crc32 < 0 || offset < 0) {
        qn_err_stor_set_invalid_chunk_put_result();
        return NULL;
    } // if

    if (!qn_json_set_string(blk_info, "ctx", ctx)) return NULL;
    if (!qn_json_set_string(blk_info, "checksum", checksum)) return NULL;
    if (!qn_json_set_string(blk_info, "host", host)) return NULL;

    if (!qn_json_set_integer(blk_info, "crc32", crc32)) return NULL;
    if (!qn_json_set_integer(blk_info, "offset", offset)) return NULL;

    // ---- Return the put result.
    return stor->obj_body;
}

QN_API qn_json_object_ptr qn_stor_rp_put_chunk(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_json_object_ptr restrict blk_info, qn_io_reader_itf restrict rdr, int chk_size, qn_stor_rput_extra_ptr restrict ext)
{
    qn_json_object_ptr put_ret;
    qn_string host;
    qn_string ctx;
    qn_string url;
    qn_rgn_entry_ptr rgn_entry;
    int chk_offset;
    int blk_size;

    // ---- Check whether need to put a chunk.
    chk_offset = qn_json_get_integer(blk_info, "offset", 0);
    blk_size = qn_json_get_integer(blk_info, "bsize", 0);

    if (chk_offset == blk_size) return qn_json_immutable_empty_object();

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    // ---- Prepare appropriate URL based on the putting stage.
    if (chk_offset == 0) {
        url = qn_cs_sprintf("%.*s/mkblk/%d", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), blk_size);
    } else {
        host = qn_json_get_string(blk_info, "host", "");
        ctx = qn_json_get_string(blk_info, "ctx", "");
        url = qn_cs_sprintf("%.*s/bput/%.*s/%d", qn_str_size(host), qn_str_cstr(host), qn_str_size(ctx), qn_str_cstr(ctx), chk_offset);
    } // if

    // ---- Do the put action.
    put_ret = qn_stor_rp_put_chunk_in_one_piece(stor, uptoken, blk_info, rdr, chk_size, url, rgn_entry, ext);
    qn_str_destroy(url);
    return put_ret;
}

QN_API qn_json_object_ptr qn_stor_rp_put_block(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_json_object_ptr restrict blk_info, qn_io_reader_itf restrict rdr, qn_stor_rput_extra_ptr restrict ext)
{
    qn_rgn_entry_ptr old_entry;
    qn_json_object_ptr put_ret;
    int chk_size;
    int chk_offset;
    int blk_size;
    int sending_bytes;
    qn_stor_rput_extra_st real_ext;

    // ---- Check whether need to put the block.
    chk_offset = qn_json_get_integer(blk_info, "offset", 0);
    blk_size = qn_json_get_integer(blk_info, "bsize", 0);

    if (chk_offset == blk_size) return qn_json_immutable_empty_object();

    // ---- Process all extra options.
    if (ext) {
        old_entry = ext->rgn_entry;
        if (!ext->rgn_entry) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &ext->rgn_entry);

        if (ext->chk_size > 0) chk_size = ext->chk_size;
    } else {
        memset(&real_ext, 0, sizeof(qn_stor_rput_extra_st));
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &real_ext.rgn_entry);
        ext = &real_ext;

        chk_size = QN_STOR_RPUT_CHUNK_DEFAULT_SIZE;
    } // if

    do {
        sending_bytes = blk_size - chk_offset;
        if (sending_bytes > chk_size) sending_bytes = chk_size;

        if (! (put_ret = qn_stor_rp_put_chunk(stor, uptoken, blk_info, rdr, sending_bytes, ext))) {
            ext->rgn_entry = old_entry;
            return NULL;
        } // if

        chk_offset += sending_bytes;
    } while (chk_offset < blk_size);

    ext->rgn_entry = old_entry;
    return put_ret;
}

static qn_json_object_ptr qn_stor_rp_make_file_to_one_piece(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_rput_session_ptr restrict ss, const qn_string restrict url, const qn_string restrict ctx_info, qn_stor_rput_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string auth_header;
    qn_string content_length;

    // ---- Reset the stor object.
    qn_stor_reset(stor);

    // ---- Set all the HTTP request headers.
    // -- Set the `Authorization` header.
    auth_header = qn_cs_sprintf("UpToken %s", uptoken);
    if (!auth_header) return NULL;

    ret = qn_http_req_set_header(stor->req, "Authorization", auth_header);
    qn_str_destroy(auth_header);
    if (!ret) return NULL;

    // -- Set the common headers.
    if (!qn_stor_prepare_common_request_headers(stor)) return NULL;

    //  -- Set the `Content-Type` and `Content-Length` headers.
    if (!qn_http_req_set_header(stor->req, "Content-Type", "text/plain")) return NULL;

    content_length = qn_cs_sprintf("%d", qn_str_size(ctx_info));
    if (!content_length) return NULL;

    ret = qn_http_req_set_header(stor->req, "Content-Length", qn_str_cstr(content_length));
    qn_str_destroy(content_length);
    if (!ret) return NULL;

    // ---- Prepare the body.
    qn_http_req_set_body_data(stor->req, qn_str_cstr(ctx_info), qn_str_size(ctx_info));

    // ---- Prepare the JSON body writer.
    if (! (stor->obj_body = qn_json_create_object())) return NULL;
    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) return NULL;
    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) return NULL;

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ---- Do the put action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    if (!ret) return NULL;
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (!qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

QN_API qn_json_object_ptr qn_stor_rp_make_file(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_rput_session_ptr restrict ss, qn_stor_rput_extra_ptr restrict ext)
{
    qn_json_object_ptr blk_info;
    qn_json_object_ptr make_ret;
    qn_integer chk_offset;
    qn_integer blk_size;
    qn_string encoded_key;
    qn_string host;
    qn_string url;
    qn_string url_tmp;
    qn_string ctx_info;
    qn_string * ctx_list;
    int blk_count;
    int i;

    blk_count = qn_stor_rs_block_count(ss);
    ctx_list = calloc(blk_count, sizeof(qn_string));
    if (!ctx_list) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    for (i = 0; i < blk_count; i += 1) {
        blk_info = qn_stor_rs_block_info(ss, i);
        if (!blk_info) {
            free(ctx_list);
            qn_err_stor_set_invalid_resumable_session_information();
            return NULL;
        } // if

        chk_offset = qn_json_get_integer(blk_info, "offset", 0);
        blk_size = qn_json_get_integer(blk_info, "bsize", 0);

        if (chk_offset > 0 && blk_size > 0 && chk_offset == blk_size) {
            ctx_list[i] = qn_json_get_string(blk_info, "ctx", NULL);
            if (!ctx_list[i]) {
                free(ctx_list);
                qn_err_stor_set_invalid_resumable_session_information();
                return NULL;
            } // if
        } else {
            free(ctx_list);
            qn_err_stor_set_invalid_resumable_session_information();
            return NULL;
        } // if
    } // for

    ctx_info = qn_str_join_list(",", ctx_list, blk_count);
    free(ctx_list);
    if (!ctx_info) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    // Here the blk_info variable is pointing to the last block.
    host = qn_json_get_string(blk_info, "host", NULL);
    if (!host) {
        qn_str_destroy(ctx_info);
        qn_err_stor_set_invalid_resumable_session_information();
        return NULL;
    } // if

    url = qn_cs_sprintf("%s/mkfile/%ld", qn_str_cstr(host), ss->fsize); // TODO: Use correct directive for large numbers.
    if (!url) {
        qn_str_destroy(ctx_info);
        return NULL;
    } // if

    if (ext->final_key) {
        encoded_key = qn_cs_encode_base64_urlsafe(ext->final_key, strlen(ext->final_key));
        if (!encoded_key) {
            qn_str_destroy(url);
            qn_str_destroy(ctx_info);
            return NULL;
        } // if

        url_tmp = qn_cs_sprintf("%s/key/%s", qn_str_cstr(url), qn_str_cstr(encoded_key));
        qn_str_destroy(encoded_key);
        qn_str_destroy(url);
        if (!url_tmp) {
            qn_str_destroy(ctx_info);
            return NULL;
        } // if

        url = url_tmp;
    } // if

    make_ret = qn_stor_rp_make_file_to_one_piece(stor, uptoken, ss, url, ctx_info, ext);
    qn_str_destroy(url);
    qn_str_destroy(ctx_info);
    return make_ret;
}

static qn_json_object_ptr qn_stor_rp_put_file_in_serial_blocks(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_rput_session_ptr restrict ss, const char * restrict fname, qn_stor_rput_extra_ptr restrict ext)
{
    qn_file_ptr fl = NULL;
    qn_io_reader_itf rdr;
    qn_json_object_ptr blk_info;
    qn_json_object_ptr put_ret = qn_json_immutable_empty_object();
    qn_integer blk_count;
    qn_integer blk_size;
    qn_integer chk_offset;
    qn_fsize start_offset;
    qn_bool skip = qn_false;
    int i;

    if (ext) {
        rdr = ext->rdr;
    } // if

    if (! rdr) {
        fl = qn_fl_open(fname, NULL);
        if (!fl) return NULL;
        rdr = qn_fl_to_io_reader(fl);
    } // if

    start_offset = 0;
    blk_count = qn_stor_rs_block_count(ss);
    for (i = 0; i < blk_count; i += 1) {
        blk_info = qn_stor_rs_block_info(ss, i);
        blk_size = qn_json_get_integer(blk_info, "bsize", 0);
        chk_offset = qn_json_get_integer(blk_info, "offset", 0);

        // TODO: Check whether the input information is all correct.

        if (chk_offset > 0 && blk_size > 0 && chk_offset == blk_size) {
            skip = qn_true;
            start_offset += blk_size;
            continue;
        } // if

        if (skip) {
            if (! qn_io_seek(rdr, start_offset)) {
                qn_fl_close(fl);
                return NULL;
            } // if
            skip = qn_false;
        } // if

        // TODO: Refresh the uptoken for every block, in the case that the deadline may expires between puts.

        put_ret = qn_stor_rp_put_block(stor, uptoken, blk_info, rdr, ext);
        if (! put_ret) {
            qn_fl_close(fl);
            return NULL;
        } // if
        start_offset += blk_size;
    } // for

    qn_fl_close(fl);
    return put_ret;
}

/***************************************************************************//**
* @ingroup Storage-Resumable-Put
*
* ## DESCRIPTION
*
* The **qn_stor_rp_put_file** function uploads a local file in a serialization
* way in multiple HTTP round-trips to put it into the destination bucket. It's
* the best way for uploading files larger than 4MB due to its capability of
* aborting and resuming via upload session and progress information.
*
* The **ss** parameter is a pointer to the pointer with the type
* **qn_stor_rput_session_ptr** to which the function saves the session information.
* In the case that the function fails, the **ss** parameter holds a session object
* and shall be passed again to the next invocation for resuming the upload session.
* Otherwise, it will be a NULL value.
*
* The client can call **qn_stor_rs_to_string()** to serialize the session object
* to a string to save to a local file, or do the reverse via **qn_stor_rs_from_string()**.
*
* @param [in] stor The pointer to the storage object.
* @param [in] auth The pointer to the authorization information. The function
*                  uses its content to archieve or generate appropriate access
*                  token.
* @param [in,out] ss The pointer to a upload session object for holding progress
*                    information (see REMARK section).
* @param [in] fname The pointer to a string specifies the path name of the
*                   local file to be uploaded.
* @param [in] ext The pointer to an extra option structure. The function uses
*                 options set in it to tune actual behaviors.
*
* ## RETURN VALUE
*
* If succeeds, the function returns a result information object contains a few of
* fields describing the meta data about the file, accompanied by some option ones
* describing other information like data-transform job ID. So it has a basic structure
* like the following one: 
*
* ~~~~~~~~{.json}
*     {
*         "fn-code": \<The HTTP code of the response\>,
*         "fn-error": "\<The HTTP message of the response\>",
*
*         "key": \<File's final name in the bucket\>,
*         "hash": "\<File's hash digest generated by Qiniu-ETAG algorithm\>",
*
*         "persistentId": \<Data tranform job ID\>
*     }
* ~~~~~~~~
*
* The **fn-code** and **fn-error** fields hold the HTTP response's status code and
* message, respectively. They are always returned by the function if the HTTP
* response returns successfully, no matter the API's operation succeeds or not.
*
* Other fields are returned only in the case that the API's operation succeeds.
*
* All HTTP codes and corresponding messages are listed as follow:
*
*     | Code  | Message                                               |
*     |-------|-------------------------------------------------------|
*     | 200   | OK.                                                   |
*     | 400   | Invalid HTTP request.                                 |
*     | 401   | Bad access token (failed in authorization check).     |
*     | 599   | Server failed due to unknown reason and CONTACT US!   |
*     | 614   | File exists.                                          |
*     | 631   | Bucket doesn't exist.                                 |
*
* **NOTE**: The caller MUST NOT destroy the result or error object because the
*           next call to storage core functions will do that.
*
* If fails, the function returns a NULL value and the caller can call
* **qn_err_get_message()** to check out what happened.
*******************************************************************************/
QN_API qn_json_object_ptr qn_stor_rp_put_file(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_rput_session_ptr * restrict ss, const char * restrict fname, qn_stor_rput_extra_ptr restrict ext)
{
    qn_rgn_entry_ptr old_entry;
    qn_json_object_ptr put_ret;
    qn_json_object_ptr make_ret;
    qn_fl_info_ptr fi;
    qn_stor_rput_extra_st real_ext;

    if (!*ss) {
        // A new session to put file.
        fi = qn_fl_info_stat(fname);
        if (!fi) return NULL;

        *ss = qn_stor_rs_create(qn_fl_info_fsize(fi));
        qn_fl_info_destroy(fi);
        if (!*ss) return NULL;
    } // if

    if (ext) {
        old_entry = ext->rgn_entry;
        if (!ext->rgn_entry) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &ext->rgn_entry);
    } else {
        memset(&real_ext, 0, sizeof(qn_stor_rput_extra_st));
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &real_ext.rgn_entry);
        ext = &real_ext;
    } // if

    put_ret = qn_stor_rp_put_file_in_serial_blocks(stor, uptoken, *ss, fname, ext);
    if (!put_ret) return NULL;
    if (qn_json_get_string(put_ret, "error", NULL)) return put_ret;

    make_ret = qn_stor_rp_make_file(stor, uptoken, *ss, ext);
    if (!make_ret) return NULL;
    if (qn_json_get_string(make_ret, "error", NULL)) return make_ret;

    qn_stor_rs_destroy(*ss);
    *ss = NULL;

    ext->rgn_entry = old_entry;
    return make_ret;
}

// -------- Resumable Upload APIs --------

typedef struct _QN_STOR_UPLOAD_PROGRESS
{
    qn_io_reader_ptr rdr_vtbl;
    qn_json_object_ptr progress;
    qn_io_reader_itf src_rdr;
    int blk_cnt;
    int ctx_idx;
    int ctx_pos;
} qn_stor_upload_progress_st;

static inline qn_stor_upload_progress_ptr qn_ctx_from_io_reader(qn_io_reader_itf restrict itf)
{
    return (qn_stor_upload_progress_ptr)( ( (char *)itf ) - (char *)( &((qn_stor_upload_progress_ptr)0)->rdr_vtbl ) );
}

static ssize_t qn_ctx_read_fn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    qn_json_array_ptr blk_arr;
    qn_json_object_ptr blk_info;
    qn_string ctx;
    qn_stor_upload_progress_ptr up = qn_ctx_from_io_reader(itf);
    char * pos = buf;
    size_t rem_size = buf_size;
    int copy_bytes;

    blk_arr = qn_json_get_array(up->progress, "blocks", NULL);

    while (rem_size > 0 && up->ctx_idx < qn_json_size_array(blk_arr)) {
        if (! (blk_info = qn_json_pick_object(blk_arr, up->ctx_idx, NULL))) {
            // TODO: Set an appropriate error.
            return -1;
        } // if

        ctx = qn_json_get_string(blk_info, "ctx", NULL);
        if (! ctx) {
            // TODO: Set an appropriate error.
            return -1;
        } // if

        if (up->ctx_idx > 0) {
            *pos++ = ',';
            rem_size -= 1;
        } // if

        copy_bytes = qn_str_size(ctx) - up->ctx_pos;
        if (rem_size < copy_bytes) copy_bytes = rem_size;

        if (copy_bytes > 0) {
            memcpy(pos, qn_str_cstr(ctx) + up->ctx_pos, copy_bytes);
            pos += copy_bytes;
            rem_size -= copy_bytes;
            up->ctx_pos += copy_bytes;

            if (up->ctx_pos == qn_str_size(ctx)) {
                up->ctx_pos = 0;
                up->ctx_idx += 1;
            } // if
        } // if
    } // while
    return buf_size - rem_size;
}

static qn_io_reader_st qn_ctx_rdr_vtable = {
    NULL, // CLOSE
    NULL, // PEEK
    &qn_ctx_read_fn,
    NULL, // SEEK
    NULL, // ADVANCE
    NULL, // DUPLICATE
    NULL, // SECTION
    NULL, // NAME
    NULL  // SIZE
};

static inline int qn_stor_up_calculate_block_count(qn_fsize fsize)
{
    return (fsize + QN_STOR_UP_BLOCK_MAX_SIZE - 1) / QN_STOR_UP_BLOCK_MAX_SIZE;
}

QN_API qn_stor_upload_progress_ptr qn_stor_up_open(const char * restrict fname, qn_fl_open_extra_ptr restrict ext)
{
    qn_file_ptr fl;
    qn_stor_upload_progress_ptr up;

    assert(fname);

    fl = qn_fl_open(fname, ext);
    if (! fl) return NULL;
    
    up = calloc(1, sizeof(qn_stor_upload_progress_st));
    if (! up) {
        qn_fl_close(fl);
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    up->progress = qn_json_create_object();
    if (! up->progress) {
        qn_fl_close(fl);
        free(up);
        return NULL;
    } // if

    up->blk_cnt = qn_stor_up_calculate_block_count(qn_fl_fsize(fl));

    if (! qn_json_set_string(up->progress, "fname", fname)) {
        qn_json_destroy_object(up->progress);
        qn_fl_close(fl);
        free(up);
        return NULL;
    } // if
    if (! qn_json_set_integer(up->progress, "bcount", up->blk_cnt)) {
        qn_json_destroy_object(up->progress);
        qn_fl_close(fl);
        free(up);
        return NULL;
    } // if
    if (! qn_json_create_and_set_array(up->progress, "blocks")) {
        qn_json_destroy_object(up->progress);
        qn_fl_close(fl);
        free(up);
        return NULL;
    } // if

    up->src_rdr = qn_fl_to_io_reader(fl);
    up->rdr_vtbl = &qn_ctx_rdr_vtable;
    return up;
}

QN_API qn_stor_upload_progress_ptr qn_stor_up_delegate(qn_io_reader_itf restrict data_rdr)
{
    qn_stor_upload_progress_ptr up;

    assert(data_rdr);

    up = calloc(1, sizeof(qn_stor_upload_progress_st));
    if (! up) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    up->progress = qn_json_create_object();
    if (! up->progress) {
        free(up);
        return NULL;
    } // if

    up->blk_cnt = qn_stor_up_calculate_block_count(qn_io_size(data_rdr));

    if (! qn_json_set_string(up->progress, "fname", qn_io_name(data_rdr))) {
        qn_json_destroy_object(up->progress);
        free(up);
        return NULL;
    } // if
    if (! qn_json_set_integer(up->progress, "bcount", up->blk_cnt)) {
        qn_json_destroy_object(up->progress);
        free(up);
        return NULL;
    } // if
    if (! qn_json_create_and_set_array(up->progress, "blocks")) {
        qn_json_destroy_object(up->progress);
        free(up);
        return NULL;
    } // if

    up->src_rdr = qn_io_duplicate(data_rdr);
    if (! up->src_rdr) {
        qn_json_destroy_object(up->progress);
        free(up);
        return NULL;
    } // if
    up->rdr_vtbl = &qn_ctx_rdr_vtable;
    return up;
}

QN_API void qn_stor_up_destroy(qn_stor_upload_progress_ptr restrict up)
{
    if (up) {
        qn_json_destroy_object(up->progress);
        qn_io_close(up->src_rdr);
        free(up);
    } // if
}

QN_API qn_string qn_stor_up_to_string(qn_stor_upload_progress_ptr restrict up)
{
    return qn_json_object_to_string(up->progress);
}

QN_API qn_stor_upload_progress_ptr qn_stor_up_from_string(const char * restrict str, size_t str_len)
{
    qn_file_ptr fl;
    qn_string fname;
    qn_stor_upload_progress_ptr up;

    assert(str && str_len > 0);

    up = calloc(1, sizeof(qn_stor_upload_progress_st));
    if (! up) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    up->progress = qn_json_object_from_string(str, str_len);
    if (! up->progress) {
        free(up);
        return NULL;
    } // if

    fname = qn_json_get_string(up->progress, "fname", NULL);
    if (! fname) {
        qn_json_destroy_object(up->progress);
        free(up);
        // TODO: Set an appropriate error.
        return NULL;
    } // if

    fl = qn_fl_open(fname, NULL);
    if (! fl) {
        qn_json_destroy_object(up->progress);
        free(up);
        return NULL;
    } // if

    up->blk_cnt = qn_json_get_integer(up->progress, "bcount", 0);
    if (up->blk_cnt == 0) up->blk_cnt = qn_stor_up_calculate_block_count(qn_fl_fsize(fl));

    up->src_rdr = qn_fl_to_io_reader(fl);
    up->rdr_vtbl = &qn_ctx_rdr_vtable;
    return up;
}

QN_API int qn_stor_up_get_block_count(qn_stor_upload_progress_ptr restrict up)
{
    assert(up);
    return up->blk_cnt;
}

QN_API qn_json_object_ptr qn_stor_up_get_block_info(qn_stor_upload_progress_ptr restrict up, int blk_idx)
{
    qn_json_array_ptr blk_arr;

    assert(up);
    assert(0 <= blk_idx);

    blk_arr = qn_json_get_array(up->progress, "blocks", NULL);
    if (blk_idx == QN_STOR_UP_BLOCK_LAST_INDEX) {
        blk_idx = qn_json_size_array(blk_arr) - 1;
    } // if
    return qn_json_pick_object(blk_arr, blk_idx, NULL);
}

QN_API qn_bool qn_stor_up_update_block_info(qn_stor_upload_progress_ptr restrict up, int blk_idx, qn_json_object_ptr restrict up_ret)
{
    qn_string ctx;
    qn_string checksum;
    qn_string host;
    qn_integer crc32;
    qn_integer offset;
    qn_integer blk_size;
    qn_json_array_ptr blk_arr;
    qn_json_object_ptr blk_info;
    qn_json_object_ptr new_blk_info;

    assert(up);
    assert(up_ret);

    if (! (blk_info = qn_stor_up_get_block_info(up, blk_idx))) return qn_false;

    blk_size = qn_json_get_integer(blk_info, "bsize", -1);
    if (blk_size < 0) {
        // TODO: Set an appropriate error.
        return qn_false;
    } // if
    
    // ---- Get and set the returned block information back into the blk_info object.
    ctx = qn_json_get_string(up_ret, "ctx", NULL);
    checksum = qn_json_get_string(up_ret, "checksum", NULL);
    host = qn_json_get_string(up_ret, "host", NULL);
    crc32 = qn_json_get_integer(up_ret, "crc32", -1);
    offset = qn_json_get_integer(up_ret, "offset", -1);

    if (! ctx || ! checksum || ! host || crc32 < 0 || offset < 0) {
        qn_err_stor_set_invalid_chunk_put_result();
        return qn_false;
    } // if

    new_blk_info = qn_json_create_object();
    if (! new_blk_info) return qn_false;

    if (! qn_json_set_string(new_blk_info, "ctx", ctx)) goto QN_STOR_UP_UPDATE_BLOCK_INFO_ERROR;
    if (! qn_json_set_string(new_blk_info, "checksum", checksum)) goto QN_STOR_UP_UPDATE_BLOCK_INFO_ERROR;
    if (! qn_json_set_string(new_blk_info, "host", host)) goto QN_STOR_UP_UPDATE_BLOCK_INFO_ERROR;
    if (! qn_json_set_integer(new_blk_info, "crc32", crc32)) goto QN_STOR_UP_UPDATE_BLOCK_INFO_ERROR;
    if (! qn_json_set_integer(new_blk_info, "offset", offset)) goto QN_STOR_UP_UPDATE_BLOCK_INFO_ERROR;
    if (! qn_json_set_integer(new_blk_info, "bsize", blk_size)) goto QN_STOR_UP_UPDATE_BLOCK_INFO_ERROR;

    blk_arr = qn_json_get_array(up->progress, "blocks", NULL);
    if (! qn_json_replace_object(blk_arr, blk_idx, new_blk_info)) goto QN_STOR_UP_UPDATE_BLOCK_INFO_ERROR;
    return qn_true;

QN_STOR_UP_UPDATE_BLOCK_INFO_ERROR:
    qn_json_destroy_object(new_blk_info);
    return qn_false;
}

QN_API qn_io_reader_itf qn_stor_up_create_block_reader(qn_stor_upload_progress_ptr restrict up, int blk_idx, qn_json_object_ptr * restrict blk_info)
{
    int i;
    int blk_size = QN_STOR_UP_BLOCK_MAX_SIZE;
    qn_json_object_ptr new_blk_info;
    qn_json_array_ptr blk_arr;

    assert(up);
    assert(0 <= blk_idx);

    blk_arr = qn_json_get_array(up->progress, "blocks", NULL);

    if (up->blk_cnt == 0) {
        // -- The source reader is a file with unknown size.
        // -- Return the very next block's reader.
        if (! (new_blk_info = qn_json_create_and_push_object(blk_arr))) return NULL;
        if (! qn_json_set_integer(new_blk_info, "bsize", blk_size)) return NULL;
        *blk_info = new_blk_info;
        return qn_io_section(up->src_rdr, 0, blk_size);
    } // if

    if (up->blk_cnt <= blk_idx) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if
    
    if (qn_json_size_array(blk_arr) <= blk_idx) {
        for (i = qn_json_size_array(blk_arr); i < blk_idx; i += 1) {
            if (! (new_blk_info = qn_json_create_and_push_object(blk_arr))) return NULL;
            if (! qn_json_set_integer(new_blk_info, "bsize", blk_size)) return NULL;
        } // for
        if (i == up->blk_cnt - 1) blk_size = (qn_io_size(up->src_rdr) % QN_STOR_UP_BLOCK_MAX_SIZE);
        if (! (new_blk_info = qn_json_create_and_push_object(blk_arr))) return NULL;
        if (! qn_json_set_integer(new_blk_info, "bsize", blk_size)) return NULL;
    } // if

    // -- The source reader is a file.
    *blk_info = new_blk_info;
    return qn_io_section(up->src_rdr, blk_idx * QN_STOR_UP_BLOCK_MAX_SIZE, blk_size);
}

static inline qn_io_reader_itf qn_stor_up_create_context_reader(qn_stor_upload_progress_ptr restrict up)
{
    up->ctx_idx = 0;
    up->ctx_pos = 0;
    return &up->rdr_vtbl;
}

static qn_string qn_stor_up_sum_fsize(qn_stor_upload_progress_ptr restrict up)
{
    qn_fsize fsize = 0;
    int i;
    qn_json_array_ptr blk_arr;

    blk_arr = qn_json_get_array(up->progress, "blocks", NULL);
    for (i = 0; i < qn_json_size_array(blk_arr); i += 1) {
        fsize += qn_json_get_integer(qn_json_pick_object(blk_arr, i, NULL), "bsize", 0);
    } // for

    // TODO: Use the correct directive depends on the type of fsize on different platforms.
    return qn_cs_sprintf("%ld", fsize);
}

static inline qn_bool qn_stor_prepare_error_info(qn_storage_ptr restrict stor)
{
    if (! (qn_json_set_integer(stor->obj_body, "fn-code", 0))) return qn_false;
    if (! (qn_json_set_string(stor->obj_body, "fn-error", "OK"))) return qn_false;
    return qn_true;
}

static inline qn_json_object_ptr qn_stor_rename_error_info(qn_storage_ptr restrict stor)
{
    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (! qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

static qn_bool qn_stor_prepare_request_for_upload(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict mime, qn_io_reader_itf restrict rdr, int size, qn_rgn_entry_ptr rgn_entry)
{
    qn_bool ret;
    qn_string tmp_hdr;

    // ---- Reset the stor object.
    qn_stor_reset(stor);

    // ---- Set all the HTTP request headers.
    // -- Set the `Authorization` header.
    tmp_hdr = qn_cs_sprintf("UpToken %s", uptoken);
    if (! tmp_hdr) return qn_false;

    ret = qn_http_req_set_header(stor->req, "Authorization", tmp_hdr);
    qn_str_destroy(tmp_hdr);
    if (! ret) return qn_false;

    // -- Set the common headers.
    if (! qn_stor_prepare_common_request_headers(stor)) return qn_false;

    //  -- Set the `Content-Type` and `Content-Length` headers.
    if (! mime) mime = "application/octet-stream";
    if (! qn_http_req_set_header(stor->req, "Content-Type", mime)) return qn_false;

    tmp_hdr = qn_cs_sprintf("%d", size);
    if (! tmp_hdr) return qn_false;

    ret = qn_http_req_set_header(stor->req, "Content-Length", tmp_hdr);
    qn_str_destroy(tmp_hdr);
    if (! ret) return qn_false;

    // ---- Prepare the request body reader.
    qn_http_req_set_body_reader(stor->req, rdr, qn_stor_upload_callback_fn, size);

    // ---- Prepare the response body writer.
    if (! (stor->obj_body = qn_json_create_object())) return qn_false;
    if (! qn_stor_prepare_error_info(stor)) return qn_false;

    qn_http_json_wrt_prepare(stor->resp_json_wrt, &stor->obj_body, NULL);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);
    return qn_true;
}

QN_API extern qn_json_object_ptr qn_stor_api_mkblk(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict blk_info, int chk_size, qn_stor_upload_extra_ptr restrict ext)
{
    int blk_size;
    qn_string url;
    qn_rgn_entry_ptr rgn_entry;

    // ---- Check preconditions.
    assert(stor);
    assert(uptoken);
    assert(blk_info);
    assert(data_rdr);

    blk_size = qn_json_get_integer(blk_info, "bsize", -1);
    if (blk_size < 0) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if

    if (chk_size <= 0) chk_size = QN_STOR_UP_CHUNK_DEFAULT_SIZE;
    if (chk_size > blk_size) chk_size = blk_size;

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (! qn_stor_prepare_request_for_upload(stor, uptoken, "application/octet-stream", data_rdr, chk_size, rgn_entry)) return NULL;

    // ---- Prepare upload URL.
    url = qn_cs_sprintf("%.*s/mkblk/%d", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), blk_size);

    // ---- Do the mkblk action.
    if (! qn_http_conn_post(stor->conn, url, stor->req, stor->resp)) return NULL;
    return qn_stor_rename_error_info(stor);
}

QN_API extern qn_json_object_ptr qn_stor_api_bput(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict blk_info, int chk_size, qn_stor_upload_extra_ptr restrict ext)
{
    qn_string url;
    qn_string host;
    qn_string ctx;
    qn_integer offset;
    qn_integer blk_size;
    qn_rgn_entry_ptr rgn_entry;

    // ---- Check preconditions.
    assert(stor);
    assert(uptoken);
    assert(blk_info);
    assert(data_rdr);

    blk_size = qn_json_get_integer(blk_info, "bsize", -1);
    if (blk_size <= 0) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if

    offset = qn_json_get_integer(blk_info, "offset", -1);
    if (offset <= 0) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if

    host = qn_json_get_string(blk_info, "host", NULL);
    if (! host) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if

    ctx = qn_json_get_string(blk_info, "ctx", NULL);
    if (! ctx) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if

    if (chk_size <= 0) chk_size = QN_STOR_UP_CHUNK_DEFAULT_SIZE;
    if (chk_size > (blk_size - offset)) chk_size = (blk_size - offset);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (! qn_stor_prepare_request_for_upload(stor, uptoken, "application/octet-stream", data_rdr, chk_size, rgn_entry)) return NULL;

    // ---- Prepare upload URL.
    url = qn_cs_sprintf("%.*s/bput/%.*s/%d", qn_str_size(host), qn_str_cstr(host), qn_str_size(ctx), qn_str_cstr(ctx), offset);

    // ---- Do the bput action.
    if (! qn_http_conn_post(stor->conn, url, stor->req, stor->resp)) return NULL;
    return qn_stor_rename_error_info(stor);
}

QN_API qn_json_object_ptr qn_stor_api_mkfile(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_upload_progress_ptr restrict up, qn_stor_upload_extra_ptr restrict ext)
{
    qn_json_object_ptr blk_info;
    qn_string url;
    qn_string host;
    qn_string done_fsize;
    int ctx_size;
    qn_rgn_entry_ptr rgn_entry;
    qn_io_reader_itf ctx_rdr;

    // ---- Check preconditions.
    assert(stor);
    assert(uptoken);
    assert(up);

    blk_info = qn_stor_up_get_block_info(up, QN_STOR_UP_BLOCK_LAST_INDEX);
    if (! blk_info) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if

    host = qn_json_get_string(blk_info, "host", NULL);
    if (! host) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if

    ctx_rdr = qn_stor_up_create_context_reader(up);
    if (! ctx_rdr) {
        // TODO: Set an appropriate error.
        return NULL;
    } // if
    ctx_size = qn_io_size(ctx_rdr);

    // ---- Process all extra options.
    if (ext) {
        if (! (rgn_entry = ext->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (! qn_stor_prepare_request_for_upload(stor, uptoken, "text/plain", ctx_rdr, ctx_size, rgn_entry)) return NULL;

    // ---- Prepare upload URL.
    done_fsize = qn_stor_up_sum_fsize(up);
    if (! done_fsize) return NULL;
    url = qn_cs_sprintf("%s/mkfile/%.*s", qn_str_cstr(host), qn_str_size(done_fsize), qn_str_cstr(done_fsize));
    qn_str_destroy(done_fsize);

    // ---- Do the mkfile action.
    if (! qn_http_conn_post(stor->conn, url, stor->req, stor->resp)) return NULL;
    return qn_stor_rename_error_info(stor);
}

#ifdef __cplusplus
}
#endif

