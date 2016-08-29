#include "qiniu/base/errors.h"
#include "qiniu/http.h"
#include "qiniu/storage.h"
#include "qiniu/misc.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_STORAGE
{
    qn_http_request_ptr req;
    qn_http_response_ptr resp;
    qn_http_connection_ptr conn;
    qn_http_json_writer_ptr resp_json_wrt;
    qn_json_object_ptr obj_body;
    qn_json_array_ptr arr_body;
} qn_storage;

qn_storage_ptr qn_stor_create(void)
{
    qn_storage_ptr new_stor = NULL;

    new_stor = calloc(1, sizeof(qn_storage));
    if (!new_stor) {
        qn_err_set_no_enough_memory();
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

void qn_stor_destroy(qn_storage_ptr stor)
{
    if (stor) {
        if (stor->obj_body) qn_json_destroy_object(stor->obj_body);
        if (stor->arr_body) qn_json_destroy_array(stor->arr_body);
        qn_http_conn_destroy(stor->conn);
        qn_http_resp_destroy(stor->resp);
        qn_http_req_destroy(stor->req);
        free(stor);
    } // if
}

qn_json_object_ptr qn_stor_get_object_body(qn_storage_ptr stor)
{
    return stor->obj_body;
}

qn_http_hdr_iterator_ptr qn_stor_resp_get_header_iterator(qn_storage_ptr stor)
{
    return qn_http_resp_get_header_iterator(stor->resp);
}

qn_bool qn_stor_stat(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, const qn_stor_query_extra_ptr ext)
{
    qn_bool ret = qn_false;
    qn_string url = NULL;
    qn_string encoded_uri = NULL;
    qn_string acctoken = NULL;
    qn_string auth_header = NULL;

    // ---- Prepare an URL
    encoded_uri = qn_misc_encode_uri(bucket, key);
    if (!encoded_uri) return qn_false;

    url = qn_str_sprintf("%s/stat/%s", "http://rs.qiniu.com", qn_str_cstr(encoded_uri));
    qn_str_destroy(encoded_uri);
    if (!url) return qn_false;

    // ---- Prepare request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (ext->acctoken) {
        auth_header = qn_str_sprintf("QBox %s", ext->acctoken);
    } else if (ext->mac) {
        acctoken = qn_mac_make_acctoken(ext->mac, url, NULL, 0);
        if (!acctoken) return qn_false;
        auth_header = qn_str_sprintf("QBox %s", acctoken);
        qn_str_destroy(acctoken);
    } // if
    if (!auth_header) {
        qn_str_destroy(url);
        return qn_false;
    } // if
    qn_http_req_set_header(stor->req, "Authorization", auth_header);
    qn_str_destroy(auth_header);

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    ret = qn_http_conn_get(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
}

// ---- Definition of upload functions

static qn_bool qn_stor_put_file_in_one_piece(qn_storage_ptr stor, const char * fname, qn_stor_put_extra_ptr ext)
{
    qn_bool ret;
    qn_string uptoken;
    qn_fl_info_ptr fi;
    qn_http_form_ptr form;

    // ---- Prepare request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    // ----
    form = qn_http_form_create();
    if (!form) return qn_false;

    // uptoken MUST be the first form item.
    if (ext->client_end.uptoken) {
        if (!qn_http_form_add_string(form, "token", ext->client_end.uptoken, strlen(ext->client_end.uptoken))) {
            qn_http_form_destroy(form);
            return qn_false;
        } // if
    } else if (ext->server_end.mac && ext->server_end.put_policy) {
        uptoken = qn_pp_to_uptoken(ext->server_end.put_policy, ext->server_end.mac);
        if (!qn_http_form_add_string(form, "token", qn_str_cstr(uptoken), qn_str_size(uptoken))) {
            qn_str_destroy(uptoken);
            qn_http_form_destroy(form);
            return qn_false;
        } // if
        qn_str_destroy(uptoken);
    } else {
        qn_err_set_invalid_argument(); 
        return qn_false;
    } // if

    if (ext->key && !qn_http_form_add_string(form, "key", ext->key, strlen(ext->key))) {
        qn_http_form_destroy(form);
        return qn_false;
    } // if

    // TODO: User defined variabales.

    if (ext->crc32 && !qn_http_form_add_string(form, "crc32", ext->crc32, strlen(ext->crc32))) {
        qn_http_form_destroy(form);
        return qn_false;
    } // if

    if (ext->accept_type && !qn_http_form_add_string(form, "accept", ext->accept_type, strlen(ext->accept_type))) {
        qn_http_form_destroy(form);
        return qn_false;
    } // if

    fi = qn_fl_info_stat(fname);
    if (!fi) {
        qn_http_form_destroy(form);
        return qn_false;
    } // if

    if (!qn_http_form_add_file(form, "file", qn_str_cstr(qn_fl_info_fname(fi)), NULL, qn_fl_info_fsize(fi))) {
        qn_fl_info_destroy(fi);
        qn_http_form_destroy(form);
        return qn_false;
    } // if
    qn_fl_info_destroy(fi);

    qn_http_req_set_form(stor->req, form);

    // ----
    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ----
    ret = qn_http_conn_post(stor->conn, "http://up.qiniu.com", stor->req, stor->resp);
    qn_http_form_destroy(form);
    return ret;
}

qn_bool qn_stor_put_file(qn_storage_ptr stor, const char * fname, qn_stor_put_extra_ptr ext)
{
    return qn_stor_put_file_in_one_piece(stor, fname, ext);
}

extern qn_bool qn_stor_put_buffer(qn_storage_ptr stor, const char * buf, qn_size buf_size, qn_stor_put_extra_ptr ext);

#ifdef __cplusplus
}
#endif

