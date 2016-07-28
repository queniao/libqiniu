#include <stdarg.h>
#include <curl/curl.h>

#include "qiniu/base/string.h"
#include "qiniu/base/errors.h"
#include "qiniu/http.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
    qn_http_htable headers;
    void * body_reader_data;
    qn_http_body_reader body_reader;
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
    req->body_reader_data = NULL;
    req->body_reader = NULL;
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

void qn_http_req_set_body_reader(qn_http_request_ptr req, void * body_reader_data, qn_http_body_reader body_reader, qn_size body_size)
{
    req->body_reader_data = body_reader_data;
    req->body_reader = body_reader;
    req->body_size = body_size;
}

// ---- Definition of HTTP response ----

typedef struct _QN_HTTP_RESPONSE
{
    int http_code;
    int writer_retcode;

    qn_http_htable headers;
    void * body_writer_data;
    qn_http_body_writer body_writer;
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
    resp->body_writer_data = NULL;
    resp->body_writer = NULL;
    qn_http_htable_reset(&resp->headers);
}

int qn_http_resp_get_code(qn_http_response_ptr resp)
{
    return resp->http_code;
}

int qn_http_resp_get_writer_retcode(qn_http_response_ptr resp)
{
    return resp->writer_retcode;
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

void qn_http_resp_set_body_writer(qn_http_response_ptr resp, void * body_writer_data, qn_http_body_writer body_writer)
{
    resp->body_writer_data = body_writer_data;
    resp->body_writer = body_writer;
}

// ---- Definition of HTTP connection ----

typedef struct _QN_HTTP_CONNECTION
{
    qn_string ip;
    int port;

    CURL * curl;
} qn_http_connection;

qn_http_connection_ptr qn_http_conn_create(const qn_string ip, int port)
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

    new_conn->ip = ip;
    new_conn->port = port;
    return new_conn;
}

void qn_http_conn_destroy(qn_http_connection_ptr conn)
{
    if (conn) {
        curl_easy_cleanup(conn->curl);
        free(conn);
    } // if
}

static qn_bool qn_http_conn_do_request(qn_http_connection_ptr conn, qn_http_request_ptr req, qn_http_response_ptr resp)
{
    CURLcode curl_code;
    struct curl_slist * headers = NULL;
    struct curl_slist * headers2 = NULL;
    qn_string entry = NULL;
    qn_http_htable_iterator itr;

    if (resp->body_writer) {
        curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION, resp->body_writer);
        curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, resp->body_writer_data);
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
    curl_easy_setopt(conn->curl, CURLOPT_READFUNCTION, req->body_reader);
    curl_easy_setopt(conn->curl, CURLOPT_READDATA, req->body_reader_data);
    curl_easy_setopt(conn->curl, CURLOPT_URL, qn_str_cstr(url));

    return qn_http_conn_do_request(conn, req, resp);
}

#ifdef __cplusplus
}
#endif

