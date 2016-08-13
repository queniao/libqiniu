#include <stdarg.h>
#include <strings.h>
#include <curl/curl.h>

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/base/errors.h"
#include "qiniu/http_header.h"
#include "qiniu/http_header_parser.h"
#include "qiniu/http.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of body reader and writer ----

typedef struct _QN_HTTP_JSON_WRITER
{
    qn_json_object_ptr * obj;
    qn_json_array_ptr * arr;
    qn_json_parser_ptr prs;
} qn_http_json_writer;

qn_http_json_writer_ptr qn_http_json_wrt_create(void)
{
    qn_http_json_writer_ptr new_body = malloc(sizeof(qn_http_json_writer));
    if (!new_body) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_body->prs = qn_json_prs_create();
    if (!new_body->prs) {
        free(new_body);
        return NULL;
    } // if
    return new_body;
}

void qn_http_json_wrt_destroy(qn_http_json_writer_ptr writer)
{
    qn_json_prs_destroy(writer->prs);
    free(writer);
}

void qn_http_json_wrt_prepare_for_object(qn_http_json_writer_ptr writer, qn_json_object_ptr * obj)
{
    writer->obj = obj;
    writer->arr = NULL;
}

void qn_http_json_wrt_prepare_for_array(qn_http_json_writer_ptr writer, qn_json_array_ptr * arr)
{
    writer->obj = NULL;
    writer->arr = arr;
}

int qn_http_json_wrt_callback(void * user_data, char * buf, int buf_size)
{
    int size = buf_size;
    qn_http_json_writer_ptr w = (qn_http_json_writer_ptr) user_data;
    if (w->obj) {
        if (!qn_json_prs_parse_object(w->prs, buf, size, w->obj)) {
            if (qn_err_is_try_again()) return size;
            return -1;
        } // if
    } else {
        if (!qn_json_prs_parse_array(w->prs, buf, size, w->arr)) {
            if (qn_err_is_try_again()) return size;
            return -1;
        } // if
    } // if
    qn_err_set_succeed();
    return buf_size;
}

// ----

typedef struct _QN_HTTP_HDR_WRITER
{
    qn_http_header_ptr hdr;
    qn_http_hdr_parser_ptr prs;
} qn_http_hdr_writer;

qn_http_hdr_writer_ptr qn_http_hdr_wrt_create(void)
{
    qn_http_hdr_writer_ptr new_writer = calloc(1, sizeof(qn_http_hdr_writer));
    if (!new_writer) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_writer->prs = qn_http_hdr_prs_create();
    if (!new_writer->prs) {
        free(new_writer);
        return NULL;
    } // if
    return new_writer;
}

void qn_http_hdr_wrt_destroy(qn_http_hdr_writer_ptr writer)
{
    if (writer) {
        qn_http_hdr_prs_destroy(writer->prs);
        free(writer);
    } // if
}

void qn_http_hdr_wrt_prepare(qn_http_hdr_writer_ptr writer, qn_http_header_ptr hdr)
{
    qn_http_hdr_prs_reset(writer->prs);
    writer->hdr = hdr;
}

int qn_http_hdr_wrt_callback(void * user_data, char * buf, int buf_size)
{
    int size = buf_size;
    qn_http_hdr_writer_ptr writer = (qn_http_hdr_writer_ptr) user_data;
    if (!qn_http_hdr_prs_parse(writer->prs, buf, &size, &writer->hdr)) {
        if (qn_err_is_try_again()) return buf_size;
        return -1;
    } // if
    qn_err_set_succeed();
    return size;
}

// ----

enum
{
    QN_HTTP_RPC_WRT_PARSING_DONE = 0,
    QN_HTTP_RPC_WRT_PARSING_HEADER,
    QN_HTTP_RPC_WRT_PARSING_JSON
};

typedef struct _QN_HTTP_RPC_WRITER
{
    int sts;
    qn_http_hdr_writer_ptr hdr_writer;
    qn_http_json_writer_ptr json_writer;
} qn_http_rpc_writer;

qn_http_rpc_writer_ptr qn_http_rpc_wrt_create(void)
{
    qn_http_rpc_writer_ptr new_writer = calloc(1, sizeof(qn_http_rpc_writer));
    if (!new_writer) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_writer->hdr_writer = qn_http_hdr_wrt_create();
    if (!new_writer->hdr_writer) {
        free(new_writer);
        return NULL;
    } // if

    new_writer->json_writer = qn_http_json_wrt_create();
    if (!new_writer->json_writer) {
        qn_http_hdr_wrt_destroy(new_writer->hdr_writer);
        free(new_writer);
        return NULL;
    } // if
    return new_writer;
}

void qn_http_rpc_wrt_destroy(qn_http_rpc_writer_ptr writer)
{
    if (writer) {
        qn_http_hdr_wrt_destroy(writer->hdr_writer);
        qn_http_json_wrt_destroy(writer->json_writer);
        free(writer);
    } // if
}

void qn_http_rpc_wrt_prepare_for_object(qn_http_rpc_writer_ptr writer, qn_http_header_ptr hdr, qn_json_object_ptr * obj)
{
    qn_http_hdr_wrt_prepare(writer->hdr_writer, hdr);
    qn_http_json_wrt_prepare_for_object(writer->json_writer, obj);
    writer->sts = QN_HTTP_RPC_WRT_PARSING_HEADER;
}

void qn_http_rpc_wrt_prepare_for_array(qn_http_rpc_writer_ptr writer, qn_http_header_ptr hdr, qn_json_array_ptr * arr)
{
    qn_http_hdr_wrt_prepare(writer->hdr_writer, hdr);
    qn_http_json_wrt_prepare_for_array(writer->json_writer, arr);
    writer->sts = QN_HTTP_RPC_WRT_PARSING_HEADER;
}

int qn_http_rpc_wrt_callback(void * user_data, char * buf, int buf_size)
{
    int ret = 0;
    int proc_size = 0;
    qn_http_rpc_writer_ptr writer = (qn_http_rpc_writer_ptr) user_data;

    switch (writer->sts) {
        case QN_HTTP_RPC_WRT_PARSING_HEADER:
            proc_size += (ret = qn_http_hdr_wrt_callback(writer->hdr_writer, buf, buf_size));
            if (ret < 0) return proc_size;
            if (qn_err_is_succeed()) writer->sts = QN_HTTP_RPC_WRT_PARSING_JSON;
            if (ret == buf_size) return buf_size;

        case QN_HTTP_RPC_WRT_PARSING_JSON:
            proc_size += (ret = qn_http_json_wrt_callback(writer->json_writer, buf + ret, buf_size - ret));
            if (ret < 0) return proc_size;
            if (qn_err_is_succeed()) writer->sts = QN_HTTP_RPC_WRT_PARSING_DONE;
        
        case QN_HTTP_RPC_WRT_PARSING_DONE:
            qn_err_set_succeed();
            return buf_size;
    } // if
    return -1;
}

// ---- Definition of HTTP request ----

typedef struct _QN_HTTP_REQUEST
{
    int body_reader_retcode;

    qn_http_header_ptr hdr;

    void * body_reader;
    qn_http_body_reader body_reader_callback;
    qn_size body_size;
} qn_http_request;

qn_http_request_ptr qn_http_req_create(void)
{
    qn_http_request_ptr new_req = NULL;

    new_req = calloc(1, sizeof(qn_http_request));
    if (!new_req) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_req->hdr = qn_http_hdr_create();
    if (!new_req->hdr) {
        free(new_req);
        return NULL;
    } // if
    return new_req;
}

void qn_http_req_destroy(qn_http_request_ptr req)
{
    if (req) {
        qn_http_hdr_destroy(req->hdr);
        free(req);
    } // if
}

void qn_http_req_reset(qn_http_request_ptr req)
{
    req->body_reader = NULL;
    req->body_reader_callback = NULL;
    qn_http_hdr_reset(req->hdr);
}

qn_bool qn_http_req_get_header_raw(qn_http_request_ptr req, const char * hdr, qn_size hdr_size, const char ** val, qn_size * val_size)
{
    return qn_http_hdr_get_raw(req->hdr, hdr, hdr_size, val, val_size);
}

qn_bool qn_http_req_set_header_with_values(qn_http_request_ptr req, const qn_string hdr, const qn_string val1, const qn_string val2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string new_val = NULL;

    va_start(ap, val2);
    new_val = qn_str_join_va("; ", val1, val2, ap);
    va_end(ap);
    if (!new_val) return qn_false;

    ret = qn_http_hdr_set(req->hdr, hdr, new_val);
    qn_str_destroy(new_val);
    return ret;
}

qn_bool qn_http_req_set_header(qn_http_request_ptr req, const qn_string hdr, const qn_string val)
{
    return qn_http_hdr_set_raw(req->hdr, qn_str_cstr(hdr), qn_str_size(hdr), qn_str_cstr(val), qn_str_size(val));
}

qn_bool qn_http_req_set_header_raw(qn_http_request_ptr req, const char * hdr, int hdr_size, const char * val, int val_size)
{
    return qn_http_hdr_set_raw(req->hdr, hdr, hdr_size, val, val_size);
}

void qn_http_req_unset_header(qn_http_request_ptr req, const qn_string hdr)
{
    qn_http_hdr_unset(req->hdr, hdr);
}

void qn_http_req_set_body_reader(qn_http_request_ptr req, void * body_reader, qn_http_body_reader body_reader_callback, qn_size body_size)
{
    req->body_reader = body_reader;
    req->body_reader_callback = body_reader_callback;
    req->body_size = body_size;
}

// ---- Definition of HTTP response ----

typedef struct _QN_HTTP_RESPONSE
{
    int http_code;
    int data_parser_retcode;

    qn_http_header_ptr hdr;
    void * data_writer;
    qn_http_data_writer data_writer_callback;
} qn_http_response;

qn_http_response_ptr qn_http_resp_create(void)
{
    qn_http_response_ptr new_resp = NULL;

    new_resp = calloc(1, sizeof(qn_http_response));
    if (!new_resp) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_resp->hdr =  qn_http_hdr_create();
    if (!new_resp->hdr) {
        free(new_resp);
        return NULL;
    } // if
    return new_resp;
}

void qn_http_resp_destroy(qn_http_response_ptr resp)
{
    if (resp) {
        qn_http_hdr_destroy(resp->hdr);
        free(resp);
    } // if
}

void qn_http_resp_reset(qn_http_response_ptr resp)
{
    resp->data_writer = NULL;
    resp->data_writer_callback = NULL;
    qn_http_hdr_reset(resp->hdr);
}

int qn_http_resp_get_code(qn_http_response_ptr resp)
{
    return resp->http_code;
}

int qn_http_resp_get_writer_retcode(qn_http_response_ptr resp)
{
    return resp->data_parser_retcode;
}

qn_bool qn_http_resp_get_header_raw(qn_http_response_ptr resp, const char * hdr, qn_size hdr_size, const char ** val, qn_size * val_size)
{
    return qn_http_hdr_get_raw(resp->hdr, hdr, hdr_size, val, val_size);
}

qn_bool qn_http_resp_set_header(qn_http_response_ptr resp, const qn_string hdr, const qn_string val)
{
    return qn_http_hdr_set_raw(resp->hdr, qn_str_cstr(hdr), qn_str_size(hdr), qn_str_cstr(val), qn_str_size(val));
}

qn_bool qn_http_resp_set_header_raw(qn_http_response_ptr resp, const char * hdr, int hdr_size, const char * val, int val_size)
{
    return qn_http_hdr_set_raw(resp->hdr, hdr, hdr_size, val, val_size);
}

void qn_http_resp_unset_header(qn_http_response_ptr resp, const qn_string hdr)
{
    qn_http_hdr_unset(resp->hdr, hdr);
}

void qn_http_resp_set_data_writer(qn_http_response_ptr resp, void * data_writer, qn_http_data_writer data_writer_callback)
{
    resp->data_writer = data_writer;
    resp->data_writer_callback = data_writer_callback;
}

// ---- Definition of HTTP connection ----

typedef struct _QN_HTTP_CONNECTION
{
    int port;
    qn_string ip;
    qn_string host;
    qn_string url_prefix;

    CURL * curl;
} qn_http_connection;

qn_http_connection_ptr qn_http_conn_create(void)
{
    qn_http_connection_ptr new_conn = NULL;

    new_conn = calloc(1, sizeof(qn_http_connection));
    if (!new_conn) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_conn->curl = curl_easy_init();
    if (!new_conn->curl) {
        // TODO: Set an appropriate error.
        free(new_conn);
        return NULL;
    }
    return new_conn;
}

void qn_http_conn_destroy(qn_http_connection_ptr conn)
{
    if (conn) {
        curl_easy_cleanup(conn->curl);
        free(conn);
    } // if
}

static size_t qn_http_conn_body_reader(char * ptr, size_t size, size_t nmemb, void * user_data)
{
    qn_http_request_ptr req = (qn_http_request_ptr) user_data;
    req->body_reader_retcode = req->body_reader_callback(req->body_reader, ptr, size * nmemb);
    if (req->body_reader_retcode != 0) return 0;
    return size * nmemb;
}

static size_t qn_http_conn_data_parser(char * ptr, size_t size, size_t nmemb, void * user_data)
{
    qn_http_response_ptr resp = (qn_http_response_ptr) user_data;
    resp->data_parser_retcode = resp->data_writer_callback(resp->data_writer, ptr, size * nmemb);
    if (resp->data_parser_retcode != 0) return 0;
    return size * nmemb;
}

static qn_bool qn_http_conn_do_request(qn_http_connection_ptr conn, qn_http_request_ptr req, qn_http_response_ptr resp)
{
    CURLcode curl_code;
    struct curl_slist * headers = NULL;
    struct curl_slist * headers2 = NULL;
    qn_string entry = NULL;
    qn_http_hdr_iterator_ptr itr;

    if (resp->data_writer_callback) {
        curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION, qn_http_conn_data_parser);
        curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, resp);
    } // if

    headers = curl_slist_append(NULL, "Expect:");
    if (!headers) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    if (qn_http_hdr_size(req->hdr) > 0) {
        itr = qn_http_hdr_itr_create(req->hdr);
        if (!itr) {
            curl_slist_free_all(headers);
            return qn_false;
        } // if

        while ((entry = qn_http_hdr_itr_next_entry(itr))) {
            headers2 = curl_slist_append(headers, entry);

            if (!headers2) {
                curl_slist_free_all(headers);
                qn_http_hdr_itr_destroy(itr);
                return qn_false;
            } // if
            headers = headers2;
        } // while

        qn_http_hdr_itr_destroy(itr);
    } // if

    curl_easy_setopt(conn->curl, CURLOPT_HTTPHEADER, headers);

    curl_code = curl_easy_perform(conn->curl);
    curl_slist_free_all(headers);

    if (curl_code != 0) {
        resp->http_code = curl_code;
        return qn_false;
    } // if

    return qn_true;
}

qn_bool qn_http_conn_get(qn_http_connection_ptr conn, const qn_string url, qn_http_request_ptr req, qn_http_response_ptr resp)
{
    curl_easy_reset(conn->curl);
    curl_easy_setopt(conn->curl, CURLOPT_POST, 0);
    curl_easy_setopt(conn->curl, CURLOPT_URL, qn_str_cstr(url));
    return qn_http_conn_do_request(conn, req, resp);
}

qn_bool qn_http_conn_post(qn_http_connection_ptr conn, const qn_string url, qn_http_request_ptr req, qn_http_response_ptr resp)
{
    curl_easy_reset(conn->curl);

    curl_easy_setopt(conn->curl, CURLOPT_POST, 1);
    curl_easy_setopt(conn->curl, CURLOPT_INFILESIZE_LARGE, req->body_size);
    curl_easy_setopt(conn->curl, CURLOPT_READFUNCTION, qn_http_conn_body_reader);
    curl_easy_setopt(conn->curl, CURLOPT_READDATA, req);
    curl_easy_setopt(conn->curl, CURLOPT_URL, qn_str_cstr(url));

    return qn_http_conn_do_request(conn, req, resp);
}

#ifdef __cplusplus
}
#endif

