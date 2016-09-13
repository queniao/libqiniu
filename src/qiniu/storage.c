#include "qiniu/base/errors.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/base/json_formatter.h"
#include "qiniu/http.h"
#include "qiniu/http_query.h"
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

QN_API qn_storage_ptr qn_stor_create(void)
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
    return qn_true;
}

// ---- Definition of Management ----

static qn_bool qn_stor_prepare_managment(qn_storage_ptr restrict stor, const qn_string restrict url, const char * restrict acctoken, const qn_mac_ptr restrict mac)
{
    qn_string auth_header;
    qn_string new_acctoken;

    if (!qn_stor_prepare_common_request_headers(stor)) return qn_false;

    if (acctoken) {
        auth_header = qn_cs_sprintf("QBox %s", acctoken);
    } else if (mac) {
        new_acctoken = qn_mac_make_acctoken(mac, url, qn_http_req_body_data(stor->req), qn_http_req_body_size(stor->req));
        if (!new_acctoken) return qn_false;

        auth_header = qn_cs_sprintf("QBox %s", new_acctoken);
        qn_str_destroy(new_acctoken);
    } else {
        qn_err_set_invalid_argument();
        return qn_false;
    } // if
    if (!auth_header) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Authorization", auth_header)) return qn_false;
    qn_str_destroy(auth_header);
    return qn_true;
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

QN_API qn_bool qn_stor_stat(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const char * restrict bucket, const char * restrict key, qn_stor_stat_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string op;
    qn_string url;

    // ---- Prepare the query URL
    op = qn_stor_make_stat_op(bucket, key);
    if (!op) return qn_false;

    url = qn_cs_sprintf("%s/%.*s", "http://rs.qiniu.com", qn_str_size(op), qn_str_cstr(op));
    qn_str_destroy(op);
    if (!url) return qn_false;

    // ---- Prepare the request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (!qn_stor_prepare_managment(stor, url, auth->client_end.acctoken, auth->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

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

QN_API qn_bool qn_stor_copy(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_copy_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string op;
    qn_string url;
    qn_string url_tmp;

    // ---- Prepare the query URL
    op = qn_stor_make_copy_op(src_bucket, src_key, dest_bucket, dest_key);
    if (!op) return qn_false;

    url = qn_cs_sprintf("%s/%.*s", "http://rs.qiniu.com", qn_str_size(op), qn_str_cstr(op));
    qn_str_destroy(op);
    if (!url) return qn_false;

    if (ext) {
        if (ext->force) {
            url_tmp = qn_cs_sprintf("%s/force/true", url);
            qn_str_destroy(url);
            if (!url_tmp) return qn_false;
            url = url_tmp;
        } // if
    } // if

    // ---- Prepare the request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_managment(stor, url, auth->client_end.acctoken, auth->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
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

QN_API qn_bool qn_stor_move(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_move_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string op;
    qn_string url;

    // ---- Prepare the query URL
    op = qn_stor_make_move_op(src_bucket, src_key, dest_bucket, dest_key);
    if (!op) return qn_false;

    url = qn_cs_sprintf("%s/%.*s", "http://rs.qiniu.com", qn_str_size(op), qn_str_cstr(op));
    qn_str_destroy(op);
    if (!url) return qn_false;

    // ---- Prepare the request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_managment(stor, url, auth->client_end.acctoken, auth->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
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

QN_API qn_bool qn_stor_delete(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const char * restrict bucket, const char * restrict key, qn_stor_delete_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string op;
    qn_string url;

    // ---- Prepare the query URL
    op = qn_stor_make_delete_op(bucket, key);
    if (!op) return qn_false;

    url = qn_cs_sprintf("%s/%.*s", "http://rs.qiniu.com", qn_str_size(op), qn_str_cstr(op));
    qn_str_destroy(op);
    if (!url) return qn_false;

    // ---- Prepare the request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_managment(stor, url, auth->client_end.acctoken, auth->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
}

QN_API qn_bool qn_stor_change_mime(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const char * restrict bucket, const char * restrict key, const char * restrict mime, qn_stor_change_mime_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_uri;
    qn_string encoded_mime;
    qn_string url;

    // ---- Prepare the query URL
    encoded_uri = qn_misc_encode_uri(bucket, key);
    if (!encoded_uri) return qn_false;

    encoded_mime = qn_cs_encode_base64_urlsafe(mime, strlen(mime));
    if (!encoded_mime) {
        qn_str_destroy(encoded_uri);
        return qn_false;
    } // if

    url = qn_cs_sprintf("%s/chgm/%s/mime/%s", "http://rs.qiniu.com", qn_str_cstr(encoded_uri), qn_str_cstr(encoded_mime));
    qn_str_destroy(encoded_uri);
    qn_str_destroy(encoded_mime);
    if (!url) return qn_false;

    // ---- Prepare the request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_managment(stor, url, auth->client_end.acctoken, auth->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
}

// ----

QN_API qn_bool qn_stor_fetch(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const char * restrict src_url, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_src_url;
    qn_string encoded_dest_uri;
    qn_string url;

    // ---- Prepare the query URL
    encoded_src_url = qn_cs_encode_base64_urlsafe(src_url, strlen(src_url));
    if (!encoded_src_url) return qn_false;

    encoded_dest_uri = qn_misc_encode_uri(dest_bucket, dest_key);
    if (!encoded_dest_uri) {
        qn_str_destroy(encoded_src_url);
        return qn_false;
    } // if

    url = qn_cs_sprintf("%s/fetch/%s/to/%s", "http://iovip.qbox.me", qn_str_cstr(encoded_src_url), qn_str_cstr(encoded_dest_uri));
    qn_str_destroy(encoded_src_url);
    qn_str_destroy(encoded_dest_uri);
    if (!url) return qn_false;

    // ---- Prepare the request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_managment(stor, url, auth->client_end.acctoken, auth->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
}

QN_API qn_bool qn_stor_prefetch(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string encoded_dest_uri;
    qn_string url;

    // ---- Prepare the query URL
    encoded_dest_uri = qn_misc_encode_uri(dest_bucket, dest_key);
    if (!encoded_dest_uri) return qn_false;

    url = qn_cs_sprintf("%s/prefetch/%s", "http://iovip.qbox.me", qn_str_cstr(encoded_dest_uri));
    qn_str_destroy(encoded_dest_uri);
    if (!url) return qn_false;

    // ---- Prepare the request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    qn_http_req_set_body_data(stor->req, "", 0);

    if (!qn_stor_prepare_managment(stor, url, auth->client_end.acctoken, auth->server_end.mac)) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    return ret;
}

// ----

QN_API qn_bool qn_stor_list(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const char * restrict bucket, qn_stor_list_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string url;
    qn_string qry_str;
    qn_string marker;
    qn_http_query_ptr qry;
    qn_json_array_ptr items;
    qn_json_object_ptr item;
    int i;
    int limit = 1000;

    qry = qn_http_qry_create();
    if (!qry) return qn_false;

    if (!qn_http_qry_set_string(qry, "bucket", bucket)) {
        qn_http_qry_destroy(qry);
        return qn_false;
    } // if

    if (ext) {
        limit = (0 < ext->limit && ext->limit <= 1000) ? ext->limit : 1000;

        if (ext->delimiter && !qn_http_qry_set_string(qry, "delimiter", ext->delimiter)) {
            qn_http_qry_destroy(qry);
            return qn_false;
        } // if

        if (ext->prefix && !qn_http_qry_set_string(qry, "prefix", ext->prefix)) {
            qn_http_qry_destroy(qry);
            return qn_false;
        } // if
    } // if

    if (!qn_http_qry_set_integer(qry, "limit", limit)) {
        qn_http_qry_destroy(qry);
        return qn_false;
    } // if

    // ---- Prepare the query URL
    do {
        if (stor->obj_body) {
            marker = qn_json_get_string(stor->obj_body, marker, qn_str_empty_string);
            if (!qn_http_qry_set_string(qry, "marker", qn_str_cstr(marker))) {
                qn_http_qry_destroy(qry);
                return qn_false;
            } // if
        } // if

        if (! (qry_str = qn_http_qry_to_string(qry))) {
            qn_http_qry_destroy(qry);
            return qn_false;
        } // if
        if (! (url = qn_cs_sprintf("%s/list?%s", "http://rsf.qbox.me", qry_str))) {
            qn_http_qry_destroy(qry);
            return qn_false;
        } // if

        // ---- Prepare the request and response
        qn_http_req_reset(stor->req);
        qn_http_resp_reset(stor->resp);

        qn_http_req_set_body_data(stor->req, "", 0);

        if (!qn_stor_prepare_managment(stor, url, auth->client_end.acctoken, auth->server_end.mac)) {
            qn_str_destroy(url);
            qn_http_qry_destroy(qry);
            return qn_false;
        } // if

        if (stor->obj_body) {
            qn_json_destroy_object(stor->obj_body);
            stor->obj_body = NULL;
        } // if

        qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
        qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

        ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
        qn_str_destroy(url);

        if (!ret || !ext->item_processor_callback) break;
        if (!stor->obj_body || qn_json_is_empty_object(stor->obj_body)) {
            // No list result.
            qn_http_qry_destroy(qry);
            return qn_true;
        } // if

        items = qn_json_get_array(stor->obj_body, "items", NULL);
        if (!items) {
            // TODO: Set an appropriate error.
            qn_http_qry_destroy(qry);
            return qn_false;
        } // if

        if (qn_json_is_empty_array(items)) {
            qn_http_qry_destroy(qry);
            return qn_true;
        } // if

        for (i = 0; i < qn_json_size_array(items); i += 1) {
            item = qn_json_pick_object(items, i, NULL);
            if (!ext->item_processor_callback(ext->item_processor, item)) {
                qn_http_qry_destroy(qry);
                return qn_false;
            } // if
        } // for
    } while (qn_json_size_array(items) == limit);

    qn_http_qry_destroy(qry);
    return ret;
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
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_bt->cap = 4;
    new_bt->ops = calloc(new_bt->cap, sizeof(qn_string));
    if (!new_bt->ops) {
        free(new_bt);
        qn_err_set_no_enough_memory();
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
        qn_err_set_no_enough_memory();
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

QN_API qn_bool qn_stor_execute_batch_opertions(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const qn_stor_batch_ptr restrict bt)
{
    qn_bool ret;
    qn_string body;
    qn_string url;

    // ---- Prepare the query URL
    body = qn_str_join_list("&", bt->ops, bt->cnt);
    if (!body) return qn_false;

    url = qn_cs_sprintf("%s/batch", "http://rs.qiniu.com");
    if (!url) {
        qn_str_destroy(body);
        return qn_false;
    } // if

    // ---- Prepare the request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    qn_http_req_set_body_data(stor->req, qn_str_cstr(body), qn_str_size(body));

    if (!qn_stor_prepare_managment(stor, url, auth->client_end.acctoken, auth->server_end.mac)) {
        qn_str_destroy(url);
        qn_str_destroy(body);
        return qn_false;
    } // if

    if (stor->arr_body) {
        qn_json_destroy_array(stor->arr_body);
        stor->arr_body = NULL;
    } // if

    qn_http_json_wrt_prepare_for_array(stor->resp_json_wrt, &stor->arr_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    qn_str_destroy(url);
    qn_str_destroy(body);
    return ret;
}

// ---- Definition of Upload ----

static qn_bool qn_stor_prepare_for_putting_file(qn_storage_ptr restrict stor, qn_stor_put_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string uptoken;
    qn_http_form_ptr form;

    // ---- Prepare request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    // ----
    if (!qn_stor_prepare_common_request_headers(stor)) return qn_false;

    // ----
    if (! (form = qn_http_req_prepare_form(stor->req))) return qn_false;

    // uptoken MUST be the first form item.
    if (ext->client_end.uptoken) {
        if (!qn_http_form_add_string(form, "token", ext->client_end.uptoken, strlen(ext->client_end.uptoken))) return qn_false;
    } else if (ext->server_end.mac && ext->server_end.put_policy) {
        uptoken = qn_pp_to_uptoken(ext->server_end.put_policy, ext->server_end.mac);
        if (!uptoken) return qn_false;

        ret = qn_http_form_add_string(form, "token", qn_str_cstr(uptoken), qn_str_size(uptoken));
        qn_str_destroy(uptoken);
        if (!ret) return qn_false;
    } else {
        qn_err_set_invalid_argument(); 
        return qn_false;
    } // if

    if (ext->final_key && !qn_http_form_add_string(form, "key", ext->final_key, strlen(ext->final_key))) return qn_false;
    if (ext->crc32 && !qn_http_form_add_string(form, "crc32", ext->crc32, strlen(ext->crc32))) return qn_false;
    if (ext->accept_type && !qn_http_form_add_string(form, "accept", ext->accept_type, strlen(ext->accept_type))) return qn_false;

    // TODO: User defined variabales.

    // ----
    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    return qn_true;
}

QN_API qn_bool qn_stor_put_file(qn_storage_ptr restrict stor, const char * restrict fname, qn_stor_put_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_fl_info_ptr fi;
    qn_http_form_ptr form;

    if (!qn_stor_prepare_for_putting_file(stor, ext)) return qn_false;

    form = qn_http_req_get_form(stor->req);

    fi = qn_fl_info_stat(fname);
    if (!fi) return qn_false;

    ret = qn_http_form_add_file(form, "file", qn_str_cstr(qn_fl_info_fname(fi)), NULL, qn_fl_info_fsize(fi));
    qn_fl_info_destroy(fi);
    if (!ret) return qn_false;

    // ----
    return qn_http_conn_post(stor->conn, "http://up.qiniu.com", stor->req, stor->resp);
}

QN_API qn_bool qn_stor_put_buffer(qn_storage_ptr restrict stor, const char * restrict buf, int buf_size, qn_stor_put_extra_ptr restrict ext)
{
    qn_http_form_ptr form;

    if (!qn_stor_prepare_for_putting_file(stor, ext)) return qn_false;

    form = qn_http_req_get_form(stor->req);

    if (!qn_http_form_add_buffer(form, "file", "<null>", buf, buf_size)) return qn_false;

    // ----
    return qn_http_conn_post(stor->conn, "http://up.qiniu.com", stor->req, stor->resp);
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
    int last_blk_size;
    qn_json_object_ptr blk_info;
    qn_stor_rput_session_ptr new_ss = calloc(1, sizeof(qn_stor_rput_session));
    if (!new_ss) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_ss->blk_info_list = qn_json_create_array();
    if (!new_ss->blk_info_list) {
        free(new_ss);
        return NULL;
    } // if
    new_ss->fsize = fsize;

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
        qn_err_set_no_enough_memory();
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

typedef struct _QN_STOR_RESUMABLE_PUT_READER
{
    qn_io_reader_ptr rdr;
    qn_stor_rput_extra_ptr ext;
} qn_stor_rput_reader, *qn_stor_rput_reader_ptr;

static size_t qn_stor_rp_chunk_body_reader_callback(void * user_data, char * buf, size_t size)
{
    size_t ret;
    qn_stor_rput_reader_ptr chk_rdr = (qn_stor_rput_reader_ptr) user_data;
    ret = chk_rdr->rdr->read(chk_rdr->rdr->user_data, buf, size);
    return ret;
}

static qn_bool qn_stor_rp_put_chunk_in_one_piece(qn_storage_ptr restrict stor, qn_json_object_ptr restrict blk_info, qn_io_reader_ptr restrict rdr, int chk_size, const qn_string restrict url, qn_stor_rput_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string uptoken;
    qn_string auth_header;
    qn_string content_length;
    qn_string ctx;
    qn_string checksum;
    qn_string host;
    qn_integer crc32;
    qn_integer offset;
    qn_io_section_reader srdr;
    qn_io_reader rdr2;
    qn_stor_rput_reader chk_rdr;

    // ---- Prepare request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    // ----
    if (ext->client_end.uptoken) {
        auth_header = qn_cs_sprintf("UpToken %s", ext->client_end.uptoken);
    } else if (ext->server_end.mac && ext->server_end.put_policy) {
        uptoken = qn_pp_to_uptoken(ext->server_end.put_policy, ext->server_end.mac);
        if (!uptoken) return qn_false;

        auth_header = qn_cs_sprintf("UpToken %s", uptoken);
        qn_str_destroy(uptoken);
    } else {
        // TODO: Set an appropriate error.
        qn_err_set_invalid_argument(); 
        return qn_false;
    } // if
    if (!auth_header) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Authorization", auth_header)) {
        qn_str_destroy(auth_header);
        return qn_false;
    } // if
    qn_str_destroy(auth_header);

    if (!qn_http_req_set_header(stor->req, "Expect", "")) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Transfer-Encoding", "")) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Content-Type", "application/octet-stream")) return qn_false;

    content_length = qn_cs_sprintf("%d", chk_size);
    if (!content_length) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Content-Length", qn_str_cstr(content_length))) {
        qn_str_destroy(content_length);
        return qn_false;
    } // if
    qn_str_destroy(content_length);

    qn_io_srd_init(&srdr, rdr, chk_size);
    rdr2.user_data = &srdr;
    rdr2.read = &qn_io_srd_read;

    memset(&chk_rdr, 0, sizeof(qn_stor_rput_reader));
    chk_rdr.rdr = &rdr2;
    chk_rdr.ext = ext;
    qn_http_req_set_body_reader(stor->req, &chk_rdr, qn_stor_rp_chunk_body_reader_callback, chk_size);

    // ----
    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ----
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    if (ret) {
        ctx = qn_json_get_string(stor->obj_body, "ctx", NULL);
        checksum = qn_json_get_string(stor->obj_body, "checksum", NULL);
        host = qn_json_get_string(stor->obj_body, "host", NULL);
        crc32 = qn_json_get_integer(stor->obj_body, "crc32", -1);
        offset = qn_json_get_integer(stor->obj_body, "offset", -1);

        if (!ctx || !checksum || !host || crc32 < 0 || offset < 0) {
            // TODO: Set an appropriate error.
            return qn_false;
        } // if

        if (!qn_json_set_string(blk_info, "ctx", ctx)) return qn_false;
        if (!qn_json_set_string(blk_info, "checksum", checksum)) return qn_false;
        if (!qn_json_set_string(blk_info, "host", host)) return qn_false;

        if (!qn_json_set_integer(blk_info, "crc32", crc32)) return qn_false;
        if (!qn_json_set_integer(blk_info, "offset", offset)) return qn_false;
    } // if
    return ret;
}

QN_API qn_bool qn_stor_rp_put_chunk(qn_storage_ptr restrict stor, qn_json_object_ptr restrict blk_info, qn_io_reader_ptr restrict rdr, int chk_size, qn_stor_rput_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string host;
    qn_string ctx;
    qn_string url;
    int chk_offset;
    int blk_size;
    
    chk_offset = qn_json_get_integer(blk_info, "offset", 0);
    blk_size = qn_json_get_integer(blk_info, "bsize", 0);

    if (chk_offset == blk_size) return qn_true;

    if (chk_offset == 0) {
        url = qn_cs_sprintf("http://up.qiniu.com/mkblk/%d", blk_size);
    } else {
        host = qn_json_get_string(blk_info, "host", "");
        ctx = qn_json_get_string(blk_info, "ctx", "");
        url = qn_cs_sprintf("%s/bput/%s/%d", host, qn_str_cstr(ctx), chk_offset);
    } // if
    ret = qn_stor_rp_put_chunk_in_one_piece(stor, blk_info, rdr, chk_size, url, ext);
    qn_str_destroy(url);
    return ret;
}

QN_API qn_bool qn_stor_rp_put_block(qn_storage_ptr restrict stor, qn_json_object_ptr restrict blk_info, qn_io_reader_ptr restrict rdr, qn_stor_rput_extra_ptr restrict ext)
{
    int chk_size;
    int chk_offset;
    int blk_size;
    int sending_bytes;

    chk_size = ext->chk_size;
    if (chk_size <= 0) chk_size = QN_STOR_RPUT_CHUNK_DEFAULT_SIZE;

    chk_offset = qn_json_get_integer(blk_info, "offset", 0);
    blk_size = qn_json_get_integer(blk_info, "bsize", 0);

    while (chk_offset < blk_size) {
        sending_bytes = blk_size - chk_offset;
        if (sending_bytes > chk_size) sending_bytes = chk_size;
        if (!qn_stor_rp_put_chunk(stor, blk_info, rdr, sending_bytes, ext)) return qn_false;
        chk_offset += sending_bytes;
    } // while
    return qn_true;
}

static qn_bool qn_stor_rp_make_file_to_one_piece(qn_storage_ptr restrict stor, qn_stor_rput_session_ptr restrict ss, const qn_string restrict url, const qn_string restrict ctx_info, qn_stor_rput_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_string uptoken;
    qn_string auth_header;
    qn_string content_length;

    // ---- Prepare request and response
    qn_http_req_reset(stor->req);
    qn_http_resp_reset(stor->resp);

    if (stor->obj_body) {
        qn_json_destroy_object(stor->obj_body);
        stor->obj_body = NULL;
    } // if

    // ----
    if (ext->client_end.uptoken) {
        auth_header = qn_cs_sprintf("UpToken %s", ext->client_end.uptoken);
    } else if (ext->server_end.mac && ext->server_end.put_policy) {
        uptoken = qn_pp_to_uptoken(ext->server_end.put_policy, ext->server_end.mac);
        if (!uptoken) return qn_false;

        auth_header = qn_cs_sprintf("UpToken %s", uptoken);
        qn_str_destroy(uptoken);
    } else {
        // TODO: Set an appropriate error;
        qn_err_set_invalid_argument(); 
        return qn_false;
    } // if
    if (!auth_header) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Authorization", auth_header)) {
        qn_str_destroy(auth_header);
        return qn_false;
    } // if
    qn_str_destroy(auth_header);

    if (!qn_http_req_set_header(stor->req, "Expect", "")) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Content-Type", "text/plain")) return qn_false;

    content_length = qn_cs_sprintf("%d", qn_str_size(ctx_info));
    if (!content_length) return qn_false;
    if (!qn_http_req_set_header(stor->req, "Content-Length", qn_str_cstr(content_length))) {
        qn_str_destroy(content_length);
        return qn_false;
    } // if
    qn_str_destroy(content_length);

    qn_http_req_set_body_data(stor->req, qn_str_cstr(ctx_info), qn_str_size(ctx_info));

    // ----
    qn_http_json_wrt_prepare_for_object(stor->resp_json_wrt, &stor->obj_body);
    qn_http_resp_set_data_writer(stor->resp, stor->resp_json_wrt, &qn_http_json_wrt_callback);

    // ----
    ret = qn_http_conn_post(stor->conn, url, stor->req, stor->resp);
    return ret;
}

QN_API qn_bool qn_stor_rp_make_file(qn_storage_ptr restrict stor, qn_stor_rput_session_ptr restrict ss, qn_stor_rput_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_json_object_ptr blk_info;
    int blk_count;
    int i;
    qn_integer chk_offset;
    qn_integer blk_size;
    qn_string encoded_key;
    qn_string host;
    qn_string url;
    qn_string url_tmp;
    qn_string ctx_info;
    qn_string * ctx_list;

    blk_count = qn_stor_rs_block_count(ss);
    ctx_list = calloc(blk_count, sizeof(qn_string));
    if (!ctx_list) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    for (i = 0; i < blk_count; i += 1) {
        blk_info = qn_stor_rs_block_info(ss, i);
        if (!blk_info) {
            // TODO: Set an apprepriate error.
            free(ctx_list);
            return qn_false;
        } // if
        
        chk_offset = qn_json_get_integer(blk_info, "offset", 0);
        blk_size = qn_json_get_integer(blk_info, "bsize", 0);

        if (chk_offset > 0 && blk_size > 0 && chk_offset == blk_size) {
            ctx_list[i] = qn_json_get_string(blk_info, "ctx", NULL);
            if (!ctx_list[i]) {
                // TODO: Set an apprepriate error.
                free(ctx_list);
                return qn_false;
            } // if
        } else {
            // TODO: Set an apprepriate error.
            free(ctx_list);
            return qn_false;
        } // if
    } // for

    ctx_info = qn_str_join_list(",", ctx_list, blk_count);
    free(ctx_list);
    if (!ctx_info) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    // Here the blk_info variable is pointing to the last block.
    host = qn_json_get_string(blk_info, "host", NULL);
    if (!host) {
        // TODO: Set an apprepriate error.
        return qn_false;
    } // if

    url = qn_cs_sprintf("%s/mkfile/%ld", qn_str_cstr(host), ss->fsize); // TODO: Use correct directive for large numbers.
    if (!url) {
        // TODO: Set an apprepriate error.
        return qn_false;
    } // if

    if (ext->final_key) {
        encoded_key = qn_cs_encode_base64_urlsafe(ext->final_key, strlen(ext->final_key));
        if (!encoded_key) return qn_false;

        url_tmp = qn_cs_sprintf("%s/key/%s", qn_str_cstr(url), qn_str_cstr(encoded_key));
        qn_str_destroy(encoded_key);
        qn_str_destroy(url);
        if (!url_tmp) return qn_false;
        url = url_tmp;
    } // if

    ret = qn_stor_rp_make_file_to_one_piece(stor, ss, url, ctx_info, ext);
    qn_str_destroy(url);
    qn_str_destroy(ctx_info);
    return ret;
}

static qn_bool qn_stor_rp_put_file_in_serial_blocks(qn_storage_ptr restrict stor, qn_stor_rput_session_ptr restrict ss, const char * restrict fname, qn_stor_rput_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_bool skip;
    qn_file_ptr fl;
    qn_json_object_ptr blk_info;
    qn_integer blk_count;
    qn_integer blk_size;
    qn_integer chk_offset;
    int i;
    qn_fsize blk_offset;
    qn_io_reader rdr;

    fl = qn_fl_open(fname, NULL);
    if (!fl) return qn_false;

    memset(&rdr, 0, sizeof(qn_io_reader));
    rdr.user_data = fl;
    rdr.read = (qn_io_read) &qn_fl_read;
    rdr.advance = (qn_io_advance) &qn_fl_advance;

    blk_offset = 0;
    blk_count = qn_stor_rs_block_count(ss);
    for (i = 0; i < blk_count; i += 1) {
        blk_info = qn_stor_rs_block_info(ss, i);
        blk_size = qn_json_get_integer(blk_info, "bsize", 0);
        chk_offset = qn_json_get_integer(blk_info, "offset", 0);

        if (chk_offset > 0 && blk_size > 0 && chk_offset == blk_size) {
            skip = qn_true;
            blk_offset += blk_size;
            continue;
        } // if

        if (skip) {
            if (!qn_fl_seek(fl, blk_offset)) {
                qn_fl_close(fl);
                return qn_false;
            } // if
            skip = qn_false;
        } // if

        ret = qn_stor_rp_put_block(stor, blk_info, &rdr, ext);
        if (!ret) {
            qn_fl_close(fl);
            return qn_false;
        } // if
        blk_offset += blk_size;
    } // for
    return qn_true;
}

QN_API qn_bool qn_stor_rp_put_file(qn_storage_ptr restrict stor, qn_stor_rput_session_ptr * restrict ss, const char * restrict fname, qn_stor_rput_extra_ptr restrict ext)
{
    qn_bool ret;
    qn_fl_info_ptr fi;

    if (!*ss) {
        // A new session to put file.
        fi = qn_fl_info_stat(fname);
        if (!fi) return qn_false;

        *ss = qn_stor_rs_create(qn_fl_info_fsize(fi));
        qn_fl_info_destroy(fi);
        if (!*ss) return qn_false;
    } // if

    if (!qn_stor_rp_put_file_in_serial_blocks(stor, *ss, fname, ext)) return qn_false;

    ret = qn_stor_rp_make_file(stor, *ss, ext);
    if (ret) {
        qn_stor_rs_destroy(*ss);
        *ss = NULL;
    } // if
    return ret;
}

#ifdef __cplusplus
}
#endif

