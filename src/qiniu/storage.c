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

// -------- Management Extra (abbreviation: mne) --------

typedef struct _QN_STOR_MANAGEMENT_EXTRA
{
    unsigned int force:1;
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_management_extra_st;

QN_API qn_stor_management_extra_ptr qn_stor_mne_create(void)
{
    qn_stor_management_extra_ptr new_se = calloc(1, sizeof(qn_stor_management_extra_st));
    if (! new_se) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_se;
}

QN_API void qn_stor_mne_destroy(qn_stor_management_extra_ptr restrict mne)
{
    if (mne) {
        free(mne);
    } // if
}

QN_API void qn_stor_mne_reset(qn_stor_management_extra_ptr restrict mne)
{
    memset(mne, 0, sizeof(qn_stor_management_extra_st));
}

QN_API void qn_stor_mne_set_force_overwrite(qn_stor_management_extra_ptr restrict mne, qn_bool force)
{
    mne->force = (force) ? 1 : 0;
}

QN_API void qn_stor_mne_set_region_entry(qn_stor_management_extra_ptr restrict mne, qn_rgn_entry_ptr restrict entry)
{
    mne->rgn_entry = entry;
}

// -------- Management Functions (abbreviation: mn) --------

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
QN_API qn_json_object_ptr qn_stor_mn_api_stat(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_management_extra_ptr restrict mne)
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
    if (mne) {
        if (! (rgn_entry = mne->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
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
QN_API qn_json_object_ptr qn_stor_mn_api_copy(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_management_extra_ptr restrict mne)
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
    if (mne) {
        if (! (rgn_entry = mne->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
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
    if (mne) {
        if (mne->force) {
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
QN_API qn_json_object_ptr qn_stor_mn_api_move(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_management_extra_ptr restrict mne)
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
    if (mne) {
        if (! (rgn_entry = mne->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
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
QN_API qn_json_object_ptr qn_stor_mn_api_delete(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_management_extra_ptr restrict mne)
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
    if (mne) {
        if (! (rgn_entry = mne->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
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
QN_API qn_json_object_ptr qn_stor_mn_api_chgm(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, const char * restrict mime, qn_stor_management_extra_ptr restrict mne)
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
    if (mne) {
        if (! (rgn_entry = mne->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
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

// -------- Batch Operations (abbreviation: bt) --------

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

// -------- Batch Functions (abbreviation: bt) --------

QN_API qn_json_object_ptr qn_stor_bt_api_batch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const qn_stor_batch_ptr restrict bt, qn_stor_management_extra_ptr restrict mne)
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
    if (mne) {
        if (! (rgn_entry = mne->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RS, NULL, &rgn_entry);
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

// -------- List Extra (abbreviation: lse) --------

typedef struct _QN_STOR_LIST_EXTRA
{
    const char * prefix;
    const char * delimiter;
    const char * marker;
    int limit;

    qn_http_query_ptr qry;
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_list_extra_st;

QN_API qn_stor_list_extra_ptr qn_stor_lse_create(void)
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

    qn_stor_lse_reset(new_le);
    return new_le;
}

QN_API void qn_stor_lse_destroy(qn_stor_list_extra_ptr restrict lse)
{
    if (lse) {
        qn_http_qry_destroy(lse->qry);
        free(lse);
    } // if
}

QN_API void qn_stor_lse_reset(qn_stor_list_extra_ptr restrict lse)
{
    lse->prefix = NULL;
    lse->delimiter = NULL;
    lse->marker = NULL;
    lse->limit = 1000;
    lse->rgn_entry = NULL;
}

QN_API void qn_stor_lse_set_prefix(qn_stor_list_extra_ptr restrict lse, const char * restrict prefix, const char * restrict delimiter)
{
    lse->prefix = prefix;
    lse->delimiter = delimiter;
}

QN_API void qn_stor_lse_set_marker(qn_stor_list_extra_ptr restrict lse, const char * restrict marker)
{
    lse->marker = marker;
}

QN_API void qn_stor_lse_set_limit(qn_stor_list_extra_ptr restrict lse, int limit)
{
    lse->limit = limit;
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
QN_API qn_json_object_ptr qn_stor_ls_api_list(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, qn_stor_list_extra_ptr restrict lse)
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
    if (lse) {
        if (! (rgn_entry = lse->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RSF, NULL, &rgn_entry);

        qry = lse->qry;

        limit = (0 < lse->limit && lse->limit <= 1000) ? lse->limit : 1000;

        if (lse->delimiter && strlen(lse->delimiter) && ! qn_http_qry_set_string(qry, "delimiter", lse->delimiter)) return NULL;
        if (lse->prefix && strlen(lse->prefix) && ! qn_http_qry_set_string(qry, "prefix", lse->prefix)) return NULL;
        if (lse->marker && strlen(lse->marker) && ! qn_http_qry_set_string(qry, "marker", lse->marker)) return NULL;
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_RSF, NULL, &rgn_entry);

        qry = qn_http_qry_create();
        if (! qry) return NULL;
    } // if

    if (! qn_http_qry_set_string(qry, "bucket", bucket)) {
        if (! lse) qn_http_qry_destroy(qry);
        return NULL;
    } // if

    if (! qn_http_qry_set_integer(qry, "limit", limit)) {
        if (! lse) qn_http_qry_destroy(qry);
        return NULL;
    } // if

    qry_str = qn_http_qry_to_string(qry);
    if (! lse) qn_http_qry_destroy(qry);
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

// -------- Fetch Extra (abbreviation: fte) --------

typedef struct _QN_STOR_FETCH_EXTRA
{
    qn_rgn_entry_ptr rgn_entry;
} qn_stor_fetch_extra_st;

QN_API qn_stor_fetch_extra_ptr qn_stor_fte_create(void)
{
    qn_stor_fetch_extra_ptr new_fe = calloc(1, sizeof(qn_stor_fetch_extra_st));
    if (! new_fe) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_fe;
}

QN_API void qn_stor_fte_destroy(qn_stor_fetch_extra_ptr restrict fte)
{
    if (fte) {
        free(fte);
    } // if
}

QN_API void qn_stor_fte_reset(qn_stor_fetch_extra_ptr restrict fte)
{
    memset(fte, 0, sizeof(qn_stor_fetch_extra_st));
}

QN_API void qn_stor_fte_set_region_entry(qn_stor_fetch_extra_ptr restrict fte, qn_rgn_entry_ptr restrict entry)
{
    fte->rgn_entry = entry;
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
QN_API qn_json_object_ptr qn_stor_ft_api_fetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_url, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict fte)
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
    if (fte) {
        if (! (rgn_entry = fte->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_IO, NULL, &rgn_entry);
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
QN_API qn_json_object_ptr qn_stor_ft_api_prefetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict fte)
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
    if (fte) {
        if (! (rgn_entry = fte->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_IO, NULL, &rgn_entry);
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

// ----

// -------- Put Policy (abbreviation: pp) --------

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

// -------- Upload Extra (abbreviation: upe) --------

typedef struct _QN_STOR_UPLOAD_EXTRA
{
    const char * final_key;
    const char * crc32;
    const char * accept_type;

    qn_rgn_entry_ptr rgn_entry;

    qn_fsize fsize;
    qn_io_reader_itf rdr;
} qn_stor_upload_extra_st;

QN_API qn_stor_upload_extra_ptr qn_stor_upe_create(void)
{
    qn_stor_upload_extra_ptr new_pe = calloc(1, sizeof(qn_stor_upload_extra_st));
    if (! new_pe) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_pe;
}

QN_API void qn_stor_upe_destroy(qn_stor_upload_extra_ptr restrict upe)
{
    if (upe) {
        free(upe);
    } // if
}

QN_API void qn_stor_upe_reset(qn_stor_upload_extra_ptr restrict upe)
{
    memset(upe, 0, sizeof(qn_stor_upload_extra_st));
}

QN_API void qn_stor_upe_set_final_key(qn_stor_upload_extra_ptr restrict upe, const char * restrict final_key)
{
    upe->final_key = final_key;
}

QN_API extern void qn_stor_upe_set_local_crc32(qn_stor_upload_extra_ptr restrict upe, const char * restrict crc32)
{
    upe->crc32 = crc32;
}

QN_API extern void qn_stor_upe_set_accept_type(qn_stor_upload_extra_ptr restrict upe, const char * restrict accept_type)
{
    upe->accept_type = accept_type;
}

QN_API extern void qn_stor_upe_set_region_entry(qn_stor_upload_extra_ptr restrict upe, qn_rgn_entry_ptr restrict entry)
{
    upe->rgn_entry = entry;
}

// -------- Ordinary Upload (abbreviation: up) --------

static qn_bool qn_stor_prepare_for_upload(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_upload_extra_ptr restrict upe)
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
    if (upe->final_key && !qn_http_form_add_string(form, "key", upe->final_key, strlen(upe->final_key))) return qn_false;
    if (upe->crc32 && !qn_http_form_add_string(form, "crc32", upe->crc32, strlen(upe->crc32))) return qn_false;
    if (upe->accept_type && !qn_http_form_add_string(form, "accept", upe->accept_type, strlen(upe->accept_type))) return qn_false;

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
QN_API qn_json_object_ptr qn_stor_up_api_upload_file(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict fname, qn_stor_upload_extra_ptr restrict upe)
{
    qn_bool ret;
    qn_fl_info_ptr fi;
    qn_http_form_ptr form;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(uptoken);

    if (upe) {
        if (! (rgn_entry = upe->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (!qn_stor_prepare_for_upload(stor, uptoken, upe)) return NULL;

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

QN_API qn_json_object_ptr qn_stor_up_api_upload_buffer(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict buf, int buf_size, qn_stor_upload_extra_ptr restrict upe)
{
    qn_bool ret;
    qn_http_form_ptr form;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(uptoken);
    assert(buf);

    if (upe) {
        if (! (rgn_entry = upe->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (!qn_stor_prepare_for_upload(stor, uptoken, upe)) return NULL;

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

QN_API qn_json_object_ptr qn_stor_up_api_upload(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_stor_upload_extra_ptr restrict upe)
{
    qn_bool ret;
    qn_rgn_entry_ptr rgn_entry;

    assert(stor);
    assert(uptoken);
    assert(data_rdr);

    if (upe) {
        if (! (rgn_entry = upe->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (! qn_stor_prepare_for_upload(stor, uptoken, upe)) return NULL;

    ret = qn_http_form_add_file_reader(qn_http_req_get_form(stor->req), "file", qn_str_cstr(qn_io_name(data_rdr)), NULL, qn_io_size(data_rdr), stor->req);
    if (! ret) return NULL;

    qn_http_req_set_body_reader(stor->req, data_rdr, qn_stor_upload_callback_fn, qn_io_size(data_rdr));

    // ----
    if (rgn_entry->hostname && !qn_http_req_set_header(stor->req, "Host", qn_str_cstr(rgn_entry->hostname))) return NULL;
    ret = qn_http_conn_post(stor->conn, qn_str_cstr(rgn_entry->base_url), stor->req, stor->resp);
    if (! ret) return NULL;

    qn_json_set_integer(stor->obj_body, "fn-code", qn_http_resp_get_code(stor->resp));
    if (! qn_json_rename(stor->obj_body, "error", "fn-error")) return (qn_err_is_no_such_entry()) ? stor->obj_body : NULL;
    return stor->obj_body;
}

// -------- Resumable Upload (abbreviation: ru) --------

typedef struct _QN_STOR_RESUMABLE_UPLOAD
{
    qn_io_reader_ptr rdr_vtbl;
    qn_json_object_ptr progress;
    qn_io_reader_itf src_rdr;
    int blk_cnt;
    int ctx_idx;
    int ctx_pos:16;
    int need_comma:1;
    qn_fsize fsize;
    qn_fsize uploaded_fsize;
} qn_stor_resumable_upload_st;

static inline qn_stor_resumable_upload_ptr qn_ctx_from_io_reader(qn_io_reader_itf restrict itf)
{
    return (qn_stor_resumable_upload_ptr)( ( (char *)itf ) - (char *)( &((qn_stor_resumable_upload_ptr)0)->rdr_vtbl ) );
}

static ssize_t qn_ctx_read_vfn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    qn_json_array_ptr blk_arr;
    qn_json_object_ptr blk_info;
    qn_string ctx;
    qn_stor_resumable_upload_ptr ru = qn_ctx_from_io_reader(itf);
    char * pos = buf;
    size_t rem_size = buf_size;
    int copy_bytes;

    blk_arr = qn_json_get_array(ru->progress, "blocks", NULL);

    while (rem_size > 0 && ru->ctx_idx < qn_json_size_array(blk_arr)) {
        if (ru->need_comma) {
            *pos++ = ',';
            rem_size -= 1;
        } // if

        blk_info = qn_json_pick_object(blk_arr, ru->ctx_idx, NULL);
        ctx = qn_json_get_string(blk_info, "ctx", NULL);
        if (! ctx) {
            qn_err_stor_set_lack_of_block_context();
            return -1;
        } // if

        copy_bytes = qn_str_size(ctx) - ru->ctx_pos;
        if (rem_size < copy_bytes) copy_bytes = rem_size;

        if (copy_bytes > 0) {
            memcpy(pos, qn_str_cstr(ctx) + ru->ctx_pos, copy_bytes);
            pos += copy_bytes;
            rem_size -= copy_bytes;
            ru->ctx_pos += copy_bytes;

            if (ru->ctx_pos == qn_str_size(ctx)) {
                ru->need_comma = 1;
                ru->ctx_pos = 0;
                ru->ctx_idx += 1;
            } // if
        } // if
    } // while
    return buf_size - rem_size;
}

static qn_fsize qn_ctx_size_vfn(qn_io_reader_itf restrict itf)
{
    qn_json_array_ptr blk_arr;
    qn_json_object_ptr blk_info;
    qn_string ctx;
    int i;
    qn_fsize size;
    qn_stor_resumable_upload_ptr ru = qn_ctx_from_io_reader(itf);

    blk_arr = qn_json_get_array(ru->progress, "blocks", NULL);

    size = 0;
    for (i = 0; i < qn_json_size_array(blk_arr); i += 1) {
        blk_info = qn_json_pick_object(blk_arr, i, NULL);
        ctx = qn_json_get_string(blk_info, "ctx", NULL);
        if (! ctx) {
            qn_err_stor_set_lack_of_block_context();
            return 0;
        } // if
        size += qn_str_size(ctx);
    } // if

    size += qn_json_size_array(blk_arr) - 1;
    return size;
}

static qn_io_reader_st qn_ctx_rdr_vtable = {
    NULL, // CLOSE
    NULL, // PEEK
    &qn_ctx_read_vfn, // READ
    NULL, // SEEK
    NULL, // ADVANCE
    NULL, // DUPLICATE
    NULL, // SECTION
    NULL, // NAME
    &qn_ctx_size_vfn  // SIZE
};

static inline int qn_stor_ru_calculate_block_count(qn_fsize fsize)
{
    return (fsize + QN_STOR_RU_BLOCK_MAX_SIZE - 1) / QN_STOR_RU_BLOCK_MAX_SIZE;
}

QN_API qn_stor_resumable_upload_ptr qn_stor_ru_create(qn_io_reader_itf restrict data_rdr)
{
    qn_stor_resumable_upload_ptr ru;

    assert(data_rdr);

    ru = calloc(1, sizeof(qn_stor_resumable_upload_st));
    if (! ru) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    ru->progress = qn_json_create_object();
    if (! ru->progress) {
        free(ru);
        return NULL;
    } // if

    ru->fsize = qn_io_size(data_rdr);
    ru->blk_cnt = qn_stor_ru_calculate_block_count(ru->fsize);

    if (! qn_json_set_integer(ru->progress, "bcount", ru->blk_cnt)) {
        qn_json_destroy_object(ru->progress);
        free(ru);
        return NULL;
    } // if
    if (! qn_json_create_and_set_array(ru->progress, "blocks")) {
        qn_json_destroy_object(ru->progress);
        free(ru);
        return NULL;
    } // if

    ru->src_rdr = qn_io_duplicate(data_rdr);
    if (! ru->src_rdr) {
        qn_json_destroy_object(ru->progress);
        free(ru);
        return NULL;
    } // if

    ru->rdr_vtbl = &qn_ctx_rdr_vtable;
    return ru;
}

QN_API void qn_stor_ru_destroy(qn_stor_resumable_upload_ptr restrict ru)
{
    if (ru) {
        qn_json_destroy_object(ru->progress);
        qn_io_close(ru->src_rdr);
        free(ru);
    } // if
}

QN_API qn_string qn_stor_ru_to_string(qn_stor_resumable_upload_ptr restrict ru)
{
    qn_bool ret;
    qn_string fsize_str;

    fsize_str = qn_cs_sprintf("%ld", ru->fsize);
    ret = qn_json_set_text(ru->progress, "fsize", qn_str_cstr(fsize_str), qn_str_size(fsize_str));
    qn_str_destroy(fsize_str);
    if (! ret) return NULL;

    fsize_str = qn_cs_sprintf("%ld", ru->uploaded_fsize);
    ret = qn_json_set_text(ru->progress, "uploaded_fsize", qn_str_cstr(fsize_str), qn_str_size(fsize_str));
    qn_str_destroy(fsize_str);
    if (! ret) return NULL;

    return qn_json_object_to_string(ru->progress);
}

QN_API qn_stor_resumable_upload_ptr qn_stor_ru_from_string(const char * restrict str, size_t str_len)
{
    qn_string fsize_str;
    qn_stor_resumable_upload_ptr ru;

    assert(str && str_len > 0);

    ru = calloc(1, sizeof(qn_stor_resumable_upload_st));
    if (! ru) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    ru->progress = qn_json_object_from_string(str, str_len);
    if (! ru->progress) {
        free(ru);
        return NULL;
    } // if

    fsize_str = qn_json_get_string(ru->progress, "fsize", NULL);
    if (! fsize_str) {
        qn_json_destroy_object(ru->progress);
        free(ru);
        qn_err_stor_set_lack_of_file_size();
        return NULL;
    } // if
    ru->fsize = atoll(qn_str_cstr(fsize_str));

    fsize_str = qn_json_get_string(ru->progress, "uploaded_fsize", NULL);
    if (! fsize_str) {
        qn_json_destroy_object(ru->progress);
        free(ru);
        qn_err_stor_set_lack_of_file_size();
        return NULL;
    } // if
    ru->uploaded_fsize = atoll(qn_str_cstr(fsize_str));

    ru->blk_cnt = qn_json_get_integer(ru->progress, "bcount", 0);
    if (ru->blk_cnt == 0 && ru->fsize > 0) {
        ru->blk_cnt = qn_stor_ru_calculate_block_count(ru->fsize);
        if (! qn_json_set_integer(ru->progress, "bcount", ru->blk_cnt)) {
            qn_json_destroy_object(ru->progress);
            free(ru);
            return NULL;
        } // if
    } // if

    ru->rdr_vtbl = &qn_ctx_rdr_vtable;
    return ru;
}

QN_API qn_bool qn_stor_ru_attach(qn_stor_resumable_upload_ptr restrict ru, qn_io_reader_itf restrict data_rdr)
{
    qn_io_reader_itf new_rdr;

    assert(ru);
    assert(data_rdr);

    new_rdr = qn_io_duplicate(data_rdr);
    if (! new_rdr) return qn_false;

    if (ru->src_rdr) {
        qn_io_close(ru->src_rdr);
        ru->src_rdr = NULL;
    } // if

    ru->src_rdr = new_rdr;
    return qn_true;
}

QN_API int qn_stor_ru_get_block_count(qn_stor_resumable_upload_ptr restrict ru)
{
    assert(ru);
    return ru->blk_cnt;
}

QN_API qn_json_object_ptr qn_stor_ru_get_block_info(qn_stor_resumable_upload_ptr restrict ru, int blk_idx)
{
    qn_json_array_ptr blk_arr;

    assert(ru);
    assert(0 <= blk_idx);

    blk_arr = qn_json_get_array(ru->progress, "blocks", NULL);
    if (blk_idx > qn_json_size_array(blk_arr)) {
        qn_err_set_out_of_range();
        return NULL;
    } // if
    if (blk_idx == QN_STOR_RU_BLOCK_LAST_INDEX) {
        blk_idx = qn_json_size_array(blk_arr) - 1;
    } // if
    return qn_json_pick_object(blk_arr, blk_idx, NULL);
}

QN_API qn_json_object_ptr qn_stor_ru_update_block_info(qn_stor_resumable_upload_ptr restrict ru, int blk_idx, qn_json_object_ptr restrict up_ret)
{
    qn_string ctx;
    qn_string checksum;
    qn_string host;
    qn_integer crc32;
    qn_integer old_offset;
    qn_integer new_offset;
    qn_integer blk_size;
    qn_json_array_ptr blk_arr;
    qn_json_object_ptr blk_info;
    qn_json_object_ptr new_blk_info;

    assert(ru);
    assert(0 <= blk_idx);
    assert(up_ret);

    if (! (blk_info = qn_stor_ru_get_block_info(ru, blk_idx))) return NULL;
    old_offset = qn_json_get_integer(blk_info, "offset", 0);

    blk_size = qn_json_get_integer(blk_info, "bsize", -1);
    
    // ---- Get and set the returned block information back into the blk_info object.
    ctx = qn_json_get_string(up_ret, "ctx", NULL);
    checksum = qn_json_get_string(up_ret, "checksum", NULL);
    host = qn_json_get_string(up_ret, "host", NULL);
    crc32 = qn_json_get_integer(up_ret, "crc32", -1);
    new_offset = qn_json_get_integer(up_ret, "offset", -1);

    if (! ctx || ! checksum || ! host || crc32 < 0 || new_offset < 0) {
        qn_err_stor_set_invalid_upload_result();
        return NULL;
    } // if

    new_blk_info = qn_json_create_object();
    if (! new_blk_info) return NULL;

    if (! qn_json_set_string(new_blk_info, "ctx", ctx)) goto QN_STOR_RU_UPDATE_BLOCK_INFO_ERROR_HANDLING;
    if (! qn_json_set_string(new_blk_info, "checksum", checksum)) goto QN_STOR_RU_UPDATE_BLOCK_INFO_ERROR_HANDLING;
    if (! qn_json_set_string(new_blk_info, "host", host)) goto QN_STOR_RU_UPDATE_BLOCK_INFO_ERROR_HANDLING;
    if (! qn_json_set_integer(new_blk_info, "crc32", crc32)) goto QN_STOR_RU_UPDATE_BLOCK_INFO_ERROR_HANDLING;
    if (! qn_json_set_integer(new_blk_info, "offset", new_offset)) goto QN_STOR_RU_UPDATE_BLOCK_INFO_ERROR_HANDLING;
    if (! qn_json_set_integer(new_blk_info, "bsize", blk_size)) goto QN_STOR_RU_UPDATE_BLOCK_INFO_ERROR_HANDLING;

    blk_arr = qn_json_get_array(ru->progress, "blocks", NULL);
    if (! qn_json_replace_object(blk_arr, blk_idx, new_blk_info)) goto QN_STOR_RU_UPDATE_BLOCK_INFO_ERROR_HANDLING;
    ru->uploaded_fsize += new_offset - old_offset;
    return new_blk_info;

QN_STOR_RU_UPDATE_BLOCK_INFO_ERROR_HANDLING:
    qn_json_destroy_object(new_blk_info);
    return NULL;
}

QN_API qn_io_reader_itf qn_stor_ru_create_block_reader(qn_stor_resumable_upload_ptr restrict ru, int blk_idx, qn_json_object_ptr * restrict blk_info)
{
    int i;
    int blk_size = QN_STOR_RU_BLOCK_MAX_SIZE;
    qn_json_object_ptr new_blk_info;
    qn_json_array_ptr blk_arr;

    assert(ru);
    assert(0 <= blk_idx);

    blk_arr = qn_json_get_array(ru->progress, "blocks", NULL);
    if (ru->blk_cnt == 0) {
        // -- The source reader is a file with unknown size.
        // -- Return the very next block's reader.
        if (! (new_blk_info = qn_json_create_and_push_object(blk_arr))) return NULL;
        if (! qn_json_set_integer(new_blk_info, "bsize", blk_size)) return NULL;
        *blk_info = new_blk_info;
        return qn_io_section(ru->src_rdr, 0, blk_size);
    } // if

    if (ru->blk_cnt <= blk_idx) {
        qn_err_set_out_of_range();
        return NULL;
    } // if
    
    if (qn_json_size_array(blk_arr) <= blk_idx) {
        for (i = qn_json_size_array(blk_arr); i < blk_idx; i += 1) {
            if (! (new_blk_info = qn_json_create_and_push_object(blk_arr))) return NULL;
            if (! qn_json_set_integer(new_blk_info, "bsize", blk_size)) return NULL;
        } // for
        if (i == ru->blk_cnt - 1) blk_size = (qn_io_size(ru->src_rdr) % QN_STOR_RU_BLOCK_MAX_SIZE);
        if (! (new_blk_info = qn_json_create_and_push_object(blk_arr))) return NULL;
        if (! qn_json_set_integer(new_blk_info, "bsize", blk_size)) return NULL;
    } // if

    // -- The source reader is a file.
    *blk_info = new_blk_info;
    return qn_io_section(ru->src_rdr, blk_idx * QN_STOR_RU_BLOCK_MAX_SIZE, blk_size);
}

QN_API qn_io_reader_itf qn_stor_ru_to_context_reader(qn_stor_resumable_upload_ptr restrict ru)
{
    ru->ctx_idx = 0;
    ru->ctx_pos = 0;
    ru->need_comma = 0;
    return &ru->rdr_vtbl;
}

QN_API qn_fsize qn_stor_ru_total_fsize(qn_stor_resumable_upload_ptr restrict ru)
{
    return ru->fsize;
}

QN_API qn_fsize qn_stor_ru_uploaded_fsize(qn_stor_resumable_upload_ptr restrict ru)
{
    return ru->uploaded_fsize;
}

QN_API qn_bool qn_stor_ru_is_block_uploaded(qn_json_object_ptr restrict blk_info)
{
    // | offset | bsize | status    |
    // | -1     | > 0   | uploading |
    // | 0      | 0     | N/A       |
    return (qn_json_get_integer(blk_info, "offset", -1) == qn_json_get_integer(blk_info, "bsize", 0));
}

QN_API qn_bool qn_stor_ru_is_file_uploaded(qn_stor_resumable_upload_ptr restrict ru)
{
    return ru->uploaded_fsize == ru->fsize;
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

QN_API extern qn_json_object_ptr qn_stor_ru_api_mkblk(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict blk_info, int chk_size, qn_stor_upload_extra_ptr restrict upe)
{
    qn_bool ret;
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
        qn_err_stor_set_lack_of_block_info();
        return NULL;
    } // if

    if (chk_size <= 0) chk_size = QN_STOR_RU_CHUNK_DEFAULT_SIZE;
    if (chk_size > blk_size) chk_size = blk_size;

    // ---- Process all extra options.
    if (upe) {
        if (! (rgn_entry = upe->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (! qn_stor_prepare_request_for_upload(stor, uptoken, "application/octet-stream", data_rdr, chk_size, rgn_entry)) return NULL;

    // ---- Prepare upload URL.
    url = qn_cs_sprintf("%.*s/mkblk/%d", qn_str_size(rgn_entry->base_url), qn_str_cstr(rgn_entry->base_url), blk_size);

    // ---- Do the mkblk action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    if (! ret) return NULL;
    return qn_stor_rename_error_info(stor);
}

QN_API extern qn_json_object_ptr qn_stor_ru_api_bput(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict blk_info, int chk_size, qn_stor_upload_extra_ptr restrict upe)
{
    qn_bool ret;
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
        qn_err_stor_set_lack_of_block_info();
        return NULL;
    } // if

    offset = qn_json_get_integer(blk_info, "offset", -1);
    if (offset <= 0) {
        qn_err_stor_set_lack_of_block_info();
        return NULL;
    } // if

    host = qn_json_get_string(blk_info, "host", NULL);
    if (! host) {
        qn_err_stor_set_lack_of_block_info();
        return NULL;
    } // if

    ctx = qn_json_get_string(blk_info, "ctx", NULL);
    if (! ctx) {
        qn_err_stor_set_lack_of_block_info();
        return NULL;
    } // if

    if (chk_size <= 0) chk_size = QN_STOR_RU_CHUNK_DEFAULT_SIZE;
    if (chk_size > (blk_size - offset)) chk_size = (blk_size - offset);

    // ---- Process all extra options.
    if (upe) {
        if (! (rgn_entry = upe->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (! qn_stor_prepare_request_for_upload(stor, uptoken, "application/octet-stream", data_rdr, chk_size, rgn_entry)) return NULL;

    // ---- Prepare upload URL.
    url = qn_cs_sprintf("%.*s/bput/%.*s/%d", qn_str_size(host), qn_str_cstr(host), qn_str_size(ctx), qn_str_cstr(ctx), offset);

    // ---- Do the bput action.
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    if (! ret) return NULL;
    return qn_stor_rename_error_info(stor);
}

QN_API qn_json_object_ptr qn_stor_ru_api_mkfile(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict ctx_rdr, qn_json_object_ptr restrict last_blk_info, qn_fsize fsize, qn_stor_upload_extra_ptr restrict upe)
{
    qn_string url;
    qn_string url_tmp;
    qn_string host;
    qn_string tmp_str;
    int ctx_size;
    qn_rgn_entry_ptr rgn_entry;

    // ---- Check preconditions.
    assert(stor);
    assert(uptoken);
    assert(ctx_rdr);
    assert(last_blk_info);

    host = qn_json_get_string(last_blk_info, "host", NULL);
    if (! host) {
        qn_err_stor_set_lack_of_block_info();
        return NULL;
    } // if

    ctx_size = qn_io_size(ctx_rdr);

    // ---- Process all extra options.
    if (upe) {
        if (! (rgn_entry = upe->rgn_entry)) qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } else {
        rgn_entry = NULL;
        qn_rgn_tbl_choose_first_entry(NULL, QN_RGN_SVC_UP, NULL, &rgn_entry);
    } // if

    if (! qn_stor_prepare_request_for_upload(stor, uptoken, "text/plain", ctx_rdr, ctx_size, rgn_entry)) return NULL;

    // ---- Prepare upload URL.
    // TODO: Use the correct directive depends on the type of fsize on different platforms.
    tmp_str = qn_cs_sprintf("%ld", fsize);
    if (! tmp_str) return NULL;
    url = qn_cs_sprintf("%.*s/mkfile/%.*s", qn_str_size(host), qn_str_cstr(host), qn_str_size(tmp_str), qn_str_cstr(tmp_str));
    qn_str_destroy(tmp_str);

    if (upe) {
        if (upe->final_key) {
            tmp_str = qn_cs_encode_base64_urlsafe(upe->final_key, strlen(upe->final_key));
            if (! tmp_str) {
                qn_str_destroy(url);
                return NULL;
            } // if

            url_tmp = qn_cs_sprintf("%.*s/key/%.*s", qn_str_size(url), qn_str_cstr(url), qn_str_size(tmp_str), qn_str_cstr(tmp_str));
            qn_str_destroy(tmp_str);
            qn_str_destroy(url);
            if (! url_tmp) return NULL;

            url = url_tmp;
        } // if
    } // if

    // ---- Do the mkfile action.
    if (! qn_http_conn_post(stor->conn, url, stor->req, stor->resp)) return NULL;
    return qn_stor_rename_error_info(stor);
}

QN_API qn_json_object_ptr qn_stor_ru_upload_huge(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_resumable_upload_ptr ru, int * start_idx, int chk_size, qn_stor_upload_extra_ptr restrict upe)
{
    int i;
    qn_integer offset;
    qn_json_object_ptr up_ret;
    qn_json_object_ptr blk_info;
    qn_io_reader_itf sec_rdr;
    qn_io_section_reader_ptr chk_rdr;

    // ---- Check preconditions.
    assert(stor);
    assert(uptoken);
    assert(ru);
    assert(0 <= *start_idx);

    // ---- Prepare internal objects.
    
    chk_rdr = qn_io_srdr_create(NULL, 0);
    if (! chk_rdr) return NULL;

    // ---- Start from the given index.
    for (i = *start_idx; i < qn_stor_ru_get_block_count(ru); i += 1) {
        blk_info = qn_stor_ru_get_block_info(ru, i);
        if (blk_info && qn_stor_ru_is_block_uploaded(blk_info)) continue;

        if (! (sec_rdr = qn_stor_ru_create_block_reader(ru, i, &blk_info))) {
            qn_io_srdr_destroy(chk_rdr);
            *start_idx = i;
            return NULL;
        } // if

        if ((offset = qn_json_get_integer(blk_info, "offset", 0)) == 0) {
            qn_io_srdr_reset(chk_rdr, sec_rdr, chk_size);
            up_ret = qn_stor_ru_api_mkblk(stor, uptoken, qn_io_srdr_to_io_reader(chk_rdr), blk_info, chk_size, upe);
            if (! up_ret || qn_json_get_integer(up_ret, "fn-code", -1) != 200) goto QN_STOR_UPLOAD_HUGE_ERROR_HANDLING;
            if (! (blk_info = qn_stor_ru_update_block_info(ru, i, up_ret))) {
                up_ret = NULL;
                goto QN_STOR_UPLOAD_HUGE_ERROR_HANDLING;
            } // if
        } else if (! qn_io_advance(sec_rdr, offset)) {
            goto QN_STOR_UPLOAD_HUGE_ERROR_HANDLING;
        } // if

        // ---- If the whole block is uploaded through the /mkblk API, skip all subsequent calls to the /bput API.
        while (! qn_stor_ru_is_block_uploaded(blk_info)) {
            qn_io_srdr_reset(chk_rdr, sec_rdr, chk_size);
            up_ret = qn_stor_ru_api_bput(stor, uptoken, qn_io_srdr_to_io_reader(chk_rdr), blk_info, chk_size, upe);
            if (! up_ret || qn_json_get_integer(up_ret, "fn-code", -1) != 200) goto QN_STOR_UPLOAD_HUGE_ERROR_HANDLING;
            if (! (blk_info = qn_stor_ru_update_block_info(ru, i, up_ret))) {
                up_ret = NULL;
                goto QN_STOR_UPLOAD_HUGE_ERROR_HANDLING;
            } // if
        } // if
        qn_io_close(sec_rdr);
    } // for

    qn_io_srdr_destroy(chk_rdr);
    *start_idx = i;
    return qn_stor_ru_api_mkfile(stor, uptoken, qn_stor_ru_to_context_reader(ru), blk_info, qn_stor_ru_uploaded_fsize(ru), upe);

QN_STOR_UPLOAD_HUGE_ERROR_HANDLING:
    qn_io_close(sec_rdr);
    qn_io_srdr_destroy(chk_rdr);
    *start_idx = i;
    return up_ret;
}

#ifdef __cplusplus
}
#endif

