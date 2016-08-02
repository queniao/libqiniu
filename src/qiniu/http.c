#include <stdarg.h>
#include <curl/curl.h>

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/base/errors.h"
#include "qiniu/http.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of body reader and writer ----

typedef struct _QN_HTTP_BODY_JSON
{
    qn_json_object_ptr * obj;
    qn_json_array_ptr * arr;
    qn_json_parser_ptr prs;
} qn_http_body_json;

qn_http_body_json_ptr qn_http_body_json_create(void)
{
    qn_http_body_json_ptr new_body = malloc(sizeof(qn_http_body_json));
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

void qn_http_body_json_destroy(qn_http_body_json_ptr writer)
{
    qn_json_prs_destroy(writer->prs);
    free(writer);
}

void qn_http_body_json_prepare_for_object(qn_http_body_json_ptr writer, qn_json_object_ptr * obj)
{
    writer->obj = obj;
    writer->arr = NULL;
}

void qn_http_body_json_prepare_for_array(qn_http_body_json_ptr writer, qn_json_array_ptr * arr)
{
    writer->obj = NULL;
    writer->arr = arr;
}

int qn_http_body_json_write(void * user_data, char * in_buf, int in_buf_size)
{
    qn_http_body_json_ptr writer = (qn_http_body_json_ptr) user_data;
    if (writer->obj) {
        if (!qn_json_prs_parse_object(writer->prs, in_buf, in_buf_size, writer->obj)) {
            return -1;
        } // if
    } else {
        if (!qn_json_prs_parse_array(writer->prs, in_buf, in_buf_size, writer->arr)) {
            return -1;
        } // if
    } // if
    return 0;
}

// ---- Definition of header table ----

typedef unsigned short qn_http_htable_pos;

typedef struct _QN_HTTP_HTABLE
{
    qn_string * entries;
    qn_http_htable_pos cnt;
    qn_http_htable_pos cap;
} qn_http_htable, *qn_http_htable_ptr;

static void qn_http_htable_reset(qn_http_htable_ptr htbl)
{
    while (htbl->cnt > 0) qn_str_destroy(htbl->entries[--htbl->cnt]);
}

static qn_bool qn_http_htable_init(qn_http_htable_ptr htbl)
{
    htbl->entries = calloc(8, sizeof(qn_string));
    if (!htbl->entries) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    htbl->cnt = 0;
    htbl->cap = 8;
    return qn_true;
}

static inline void qn_http_htable_clean(qn_http_htable_ptr htbl)
{
    qn_http_htable_reset(htbl);
    free(htbl->entries);
}

static qn_bool qn_http_htable_augment(qn_http_htable_ptr htbl)
{
    qn_http_htable_pos new_cap = htbl->cap + (htbl->cap >> 1); // 1.5 times
    qn_string * new_entries = calloc(8, sizeof(qn_string));
    if (!new_entries) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_entries, htbl->entries, sizeof(qn_string) * htbl->cnt);
    free(htbl->entries);
    htbl->entries = new_entries;
    htbl->cap = new_cap;
    return qn_true;
}

static qn_http_htable_pos qn_http_htable_bsearch(qn_http_htable_ptr htbl, const qn_string hdr, qn_size hdr_size)
{
    qn_http_htable_pos begin = 0;
    qn_http_htable_pos end = htbl->cnt;
    qn_http_htable_pos mid = 0;

    while (begin < end) {
        mid = begin + ((end - begin) / 2);
        if (strncmp(htbl->entries[mid], hdr, hdr_size) < 0) {
            begin = mid + 1;
        } else {
            end = mid;
        } // if
    } // while
    return begin;
}

static qn_bool qn_http_htable_set(qn_http_htable_ptr htbl, const qn_string hdr, const qn_string val)
{
    qn_string new_val = NULL;
    qn_http_htable_pos pos = 0;

    if (htbl->cnt == htbl->cap && !qn_http_htable_augment(htbl)) return qn_false;

    new_val = qn_str_sprintf("%s: %s", hdr, val);
    if (!new_val) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    pos = qn_http_htable_bsearch(htbl, hdr, qn_str_size(hdr));
    if (pos < htbl->cnt) {
        qn_str_destroy(htbl->entries[pos]);
        htbl->entries[pos] = new_val;
    } else {
        memmove(&htbl->entries[pos+1], &htbl->entries[pos], sizeof(qn_string) * (htbl->cnt - pos));
        htbl->entries[pos] = new_val;
        htbl->cnt += 1;
    } // if
    return qn_true;
}

static inline void qn_http_htable_unset(qn_http_htable_ptr htbl, const qn_string hdr)
{
    qn_http_htable_pos pos = qn_http_htable_bsearch(htbl, hdr, qn_str_size(hdr));
    if (pos < htbl->cnt) {
        qn_str_destroy(htbl->entries[pos]);
        htbl->cnt -= 1;
    } // if
}

static inline const qn_string qn_http_htable_get(qn_http_htable_ptr htbl, const qn_string hdr)
{
    qn_size hdr_size = qn_str_size(hdr);
    qn_http_htable_pos pos = qn_http_htable_bsearch(htbl, hdr, hdr_size);
    return (pos < htbl->cnt) ? htbl->entries[pos] + hdr_size + 2 : NULL;
}

static inline const qn_string qn_http_htable_get_entry(qn_http_htable_ptr htbl, const qn_string hdr)
{
    qn_http_htable_pos pos = qn_http_htable_bsearch(htbl, hdr, qn_str_size(hdr));
    return (pos < htbl->cnt) ? htbl->entries[pos] : NULL;
}

typedef struct _QN_HTTP_HEADER_TABLE_ITERATOR
{
    qn_http_htable_ptr htbl;
    qn_http_htable_pos pos;
} qn_http_htable_iterator, *qn_http_htable_iterator_ptr;

static inline qn_http_htable_iterator qn_http_htable_make_iterator(qn_http_htable_ptr htbl)
{
    qn_http_htable_iterator new_itr;
    new_itr.htbl = htbl;
    new_itr.pos = 0;
    return new_itr;
}

static inline const qn_string qn_http_htable_next_entry(qn_http_htable_iterator_ptr itr)
{
    return (itr->pos < itr->htbl->cnt) ? itr->htbl->entries[itr->pos++] : NULL;
}

// ---- Definition of HTTP request ----

typedef struct _QN_HTTP_REQUEST
{
    int body_reader_retcode;

    qn_http_htable headers;

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

    if (!qn_http_htable_init(&new_req->headers)) {
        free(new_req);
        return NULL;
    } // if
    return new_req;
}

void qn_http_req_destroy(qn_http_request_ptr req)
{
    if (req) {
        qn_http_htable_clean(&req->headers);
        free(req);
    } // if
}

void qn_http_req_reset(qn_http_request_ptr req)
{
    req->body_reader = NULL;
    req->body_reader_callback = NULL;
    qn_http_htable_reset(&req->headers);
}

qn_string qn_http_req_get_header(qn_http_request_ptr req, const qn_string hdr)
{
    return qn_http_htable_get(&req->headers, hdr);
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

    ret = qn_http_htable_set(&req->headers, hdr, new_val);
    qn_str_destroy(new_val);
    return ret;
}

qn_bool qn_http_req_set_header(qn_http_request_ptr req, const qn_string hdr, const qn_string val)
{
    return qn_http_htable_set(&req->headers, hdr, val);
}

void qn_http_req_unset_header(qn_http_request_ptr req, const qn_string hdr)
{
    qn_http_htable_unset(&req->headers, hdr);
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
    int body_writer_retcode;

    qn_http_htable headers;
    void * body_writer;
    qn_http_body_writer body_writer_callback;
} qn_http_response;

qn_http_response_ptr qn_http_resp_create(void)
{
    qn_http_response_ptr new_resp = NULL;

    new_resp = calloc(1, sizeof(qn_http_response));
    if (!new_resp) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    if (!qn_http_htable_init(&new_resp->headers)) {
        free(new_resp);
        return NULL;
    } // if
    return new_resp;
}

void qn_http_resp_destroy(qn_http_response_ptr resp)
{
    if (resp) {
        qn_http_htable_clean(&resp->headers);
        free(resp);
    } // if
}

void qn_http_resp_reset(qn_http_response_ptr resp)
{
    resp->body_writer = NULL;
    resp->body_writer_callback = NULL;
    qn_http_htable_reset(&resp->headers);
}

int qn_http_resp_get_code(qn_http_response_ptr resp)
{
    return resp->http_code;
}

int qn_http_resp_get_writer_retcode(qn_http_response_ptr resp)
{
    return resp->body_writer_retcode;
}

qn_string qn_http_resp_get_header(qn_http_response_ptr resp, const qn_string hdr)
{
    return qn_http_htable_get(&resp->headers, hdr);
}

qn_bool qn_http_resp_set_header(qn_http_response_ptr resp, const qn_string hdr, const qn_string val)
{
    return qn_http_htable_set(&resp->headers, hdr, val);
}

void qn_http_resp_unset_header(qn_http_response_ptr resp, const qn_string hdr)
{
    qn_http_htable_unset(&resp->headers, hdr);
}

void qn_http_resp_set_body_writer(qn_http_response_ptr resp, void * body_writer, qn_http_body_writer body_writer_callback)
{
    resp->body_writer = body_writer;
    resp->body_writer_callback = body_writer_callback;
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
    if (req->body_reader_retcode != 0) {
        return 0;
    } // if
    return size * nmemb;
}

static size_t qn_http_conn_body_writer(char * ptr, size_t size, size_t nmemb, void * user_data)
{
    qn_http_response_ptr resp = (qn_http_response_ptr) user_data;
    resp->body_writer_retcode = resp->body_writer_callback(resp->body_writer, ptr, size * nmemb);
    if (resp->body_writer_retcode != 0) {
        return 0;
    } // if
    return size * nmemb;
}

static qn_bool qn_http_conn_do_request(qn_http_connection_ptr conn, qn_http_request_ptr req, qn_http_response_ptr resp)
{
    CURLcode curl_code;
    struct curl_slist * headers = NULL;
    struct curl_slist * headers2 = NULL;
    qn_string entry = NULL;
    qn_http_htable_iterator itr;

    if (resp->body_writer_callback) {
        curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION, qn_http_conn_body_writer);
        curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, resp);
    } // if

    headers = curl_slist_append(NULL, "Expect:");
    if (!headers) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    itr = qn_http_htable_make_iterator(&req->headers);
    while ((entry = qn_http_htable_next_entry(&itr))) {
        headers2 = curl_slist_append(headers, entry);

        if (!headers2) {
            curl_slist_free_all(headers);
            return qn_false;
        } // if
        headers = headers2;
    } // while
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

