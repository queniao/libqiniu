#include <stdarg.h>
#include <strings.h>
#include <ctype.h>
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

typedef enum _QN_HTTP_JSON_WRT_STATUS
{
    QN_HTTP_JSON_WRT_PARSING_READY = 0,
    QN_HTTP_JSON_WRT_PARSING_OBJECT = 1,
    QN_HTTP_JSON_WRT_PARSING_ARRAY = 2,
    QN_HTTP_JSON_WRT_PARSING_DONE = 3,
    QN_HTTP_JSON_WRT_PARSING_ERROR = 4
} qn_http_json_wrt_status;

typedef struct _QN_HTTP_JSON_WRITER
{
    qn_http_json_wrt_status sts;
    qn_json_object_ptr * obj;
    qn_json_array_ptr * arr;
    qn_json_parser_ptr prs;
} qn_http_json_writer;

QN_SDK qn_http_json_writer_ptr qn_http_json_wrt_create(void)
{
    qn_http_json_writer_ptr new_body = malloc(sizeof(qn_http_json_writer));
    if (!new_body) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_body->prs = qn_json_prs_create();
    if (!new_body->prs) {
        free(new_body);
        return NULL;
    } // if
    return new_body;
}

QN_SDK void qn_http_json_wrt_destroy(qn_http_json_writer_ptr restrict writer)
{
    qn_json_prs_destroy(writer->prs);
    free(writer);
}

QN_SDK void qn_http_json_wrt_prepare(qn_http_json_writer_ptr restrict writer, qn_json_object_ptr * restrict obj, qn_json_array_ptr * restrict arr)
{
    writer->obj = obj;
    writer->arr = arr;
    writer->sts = QN_HTTP_JSON_WRT_PARSING_READY;
}

static size_t qn_http_json_wrt_parse_object(qn_http_json_writer_ptr w, char * restrict buf, size_t buf_size)
{
    size_t size = buf_size;
    if (qn_json_prs_parse_object(w->prs, buf, &size, w->obj)) {
        // ---- Parsing object is done.
        w->sts = QN_HTTP_JSON_WRT_PARSING_DONE;
        qn_err_set_succeed();
        return buf_size;
    } // if

    // ---- Handle errors.
    if (qn_err_json_is_need_more_text_input()) {
        w->sts = QN_HTTP_JSON_WRT_PARSING_OBJECT;
        return buf_size;
    } // if

    // ---- Parsing object failed in other chunks of the body.
    if (!qn_err_json_is_bad_text_input()) w->sts = QN_HTTP_JSON_WRT_PARSING_ERROR;
    return 0;
}

static size_t qn_http_json_wrt_parse_array(qn_http_json_writer_ptr w, char * restrict buf, size_t buf_size)
{
    size_t size = buf_size;
    if (qn_json_prs_parse_array(w->prs, buf, &size, w->arr)) {
        // ---- Parsing array is done.
        w->sts = QN_HTTP_JSON_WRT_PARSING_DONE;
        qn_err_set_succeed();
        return buf_size;
    } // if

    // ---- Handle errors.
    if (qn_err_json_is_need_more_text_input()) {
        w->sts = QN_HTTP_JSON_WRT_PARSING_ARRAY;
        return buf_size;
    } // if

    // ---- Parsing array failed in other chunks of the body.
    if (!qn_err_json_is_bad_text_input()) w->sts = QN_HTTP_JSON_WRT_PARSING_ERROR;
    return 0;
}

QN_SDK size_t qn_http_json_wrt_write_cfn(void * user_data, char * restrict buf, size_t buf_size)
{
    size_t ret;
    qn_http_json_writer_ptr w = (qn_http_json_writer_ptr) user_data;

    switch (w->sts) {
        case QN_HTTP_JSON_WRT_PARSING_READY:
            // ---- Try to parse as a JSON object first.
            ret = qn_http_json_wrt_parse_object(w, buf, buf_size);

            if (w->sts == QN_HTTP_JSON_WRT_PARSING_READY) {
                // ---- If the first chunk of the body does not start a JSON object,
                //      try to parse as a JSON array.

                ret = qn_http_json_wrt_parse_array(w, buf, buf_size);
                if (w->sts == QN_HTTP_JSON_WRT_PARSING_READY) {
                    qn_err_json_set_bad_text_input();
                    ret = 0;
                } // if
            } // if
            return ret;

        case QN_HTTP_JSON_WRT_PARSING_OBJECT: return qn_http_json_wrt_parse_object(w, buf, buf_size);
        case QN_HTTP_JSON_WRT_PARSING_ARRAY: return qn_http_json_wrt_parse_array(w, buf, buf_size);
        case QN_HTTP_JSON_WRT_PARSING_DONE: ret = buf_size; break;
        case QN_HTTP_JSON_WRT_PARSING_ERROR: ret = 0; break;
    } // switch
    return buf_size;
}

// ---- Definition of HTTP form

typedef struct _QN_HTTP_FORM
{
    struct curl_httppost * first;
    struct curl_httppost * last;
} qn_http_form;

QN_SDK qn_http_form_ptr qn_http_form_create(void)
{
    qn_http_form_ptr new_form = calloc(1, sizeof(qn_http_form));
    if (!new_form) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_form;
}

QN_SDK void qn_http_form_destroy(qn_http_form_ptr restrict form)
{
    if (form) {
        qn_http_form_reset(form);
        free(form);
    } // form
}

QN_SDK void qn_http_form_reset(qn_http_form_ptr restrict form)
{
    curl_formfree(form->first);
    form->first = NULL;
    form->last = NULL;
}

QN_SDK qn_bool qn_http_form_add_raw(qn_http_form_ptr restrict form, const char * restrict fld, qn_size fld_size, const char * restrict val, qn_size val_size)
{
    CURLFORMcode ret;

    if (sizeof(curl_off_t) < sizeof(qn_size)) {
        if (UINT32_MAX < fld_size || UINT32_MAX < val_size) {
            qn_err_set_overflow_upper_bound();
            return qn_false;
        } // if
    } // if
    
    ret = curl_formadd(&form->first, &form->last, CURLFORM_COPYNAME, fld, CURLFORM_NAMELENGTH, fld_size, CURLFORM_COPYCONTENTS, val, CURLFORM_CONTENTLEN, val_size, CURLFORM_END);
    if (ret != 0) {
        qn_err_http_set_adding_string_field_failed();
        return qn_false;
    } // if
    return qn_true;
}

static inline const char * qn_http_get_fname_utf8(const char * restrict fname)
{
    const char * fname_utf8;
    if (!fname) return "LIBQINIU-MANDATORY-FILENAME";
#ifdef QN_OS_WINDOWS
    return ((fname_utf8 = strrchr(fname, '\\'))) ? fname_utf8 + 1 : fname;
#else
    return ((fname_utf8 = strrchr(fname, '/'))) ? fname_utf8 + 1 : fname;
#endif
}

QN_SDK qn_bool qn_http_form_add_file(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict fname_utf8, qn_fsize fsize, const char * restrict mime_type)
{
    CURLFORMcode ret;

    /// BUG NOTE 1 : Golang HTTP server will fail in case that the fsize is larger than 10MB and the `filename` attribute of the multipart-data section doesn't exist.
    /// BUG FIX    : Use a mandatory filename value to prevent Golang HTTP server from failing.
    if (!fname_utf8) fname_utf8 = qn_http_get_fname_utf8(fname);
    if (! mime_type) mime_type = "application/octet-stream";

    ret = curl_formadd(&form->first, &form->last, CURLFORM_COPYNAME, field, CURLFORM_FILE, fname, CURLFORM_FILENAME, fname_utf8, CURLFORM_CONTENTTYPE, mime_type, CURLFORM_END);
    if (ret != 0) {
        qn_err_http_set_adding_file_field_failed();
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_http_form_add_file_reader(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict fname_utf8, qn_fsize fsize, const char * restrict mime_type, void * restrict req)
{
    CURLFORMcode ret;

    /// See BUG NOTE 1.
    if (!fname_utf8) fname_utf8 = qn_http_get_fname_utf8(fname);
    if (! mime_type) mime_type = "application/octet-stream";

    ret = curl_formadd(&form->first, &form->last, CURLFORM_COPYNAME, field, CURLFORM_STREAM, req, CURLFORM_CONTENTSLENGTH, (long)fsize, CURLFORM_FILENAME, fname_utf8, CURLFORM_CONTENTTYPE, mime_type, CURLFORM_END);
    if (ret != 0) {
        qn_err_http_set_adding_file_field_failed();
        return qn_false;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_http_form_add_buffer(qn_http_form_ptr restrict form, const char * restrict field, const char * restrict fname, const char * restrict buf, qn_size buf_size, const char * restrict mime_type)
{
    CURLFORMcode ret;

    if (sizeof(long) < sizeof(qn_size)) {
        if (UINT32_MAX < buf_size) {
            qn_err_set_overflow_upper_bound();
            return qn_false;
        } // if
    } // if

    if (! mime_type) mime_type = "application/octet-stream";
    
    ret = curl_formadd(&form->first, &form->last, CURLFORM_COPYNAME, field, CURLFORM_BUFFER, fname, CURLFORM_BUFFERPTR, buf, CURLFORM_BUFFERLENGTH, buf_size, CURLFORM_CONTENTTYPE, mime_type, CURLFORM_END);
    if (ret != 0) {
        qn_err_http_set_adding_buffer_field_failed();
        return qn_false;
    } // if
    return qn_true;
}

// ---- Definition of HTTP request ----

enum
{
    QN_HTTP_REQ_USING_LOCAL_FORM = 0x1
};

typedef struct _QN_HTTP_REQUEST
{
    int flags;
    qn_http_header_ptr hdr;
    qn_http_form_ptr form;

    const char * body_data;
    qn_fsize body_size;

    void * body_rdr;
    qn_http_body_reader_callback_fn body_rdr_cb;
} qn_http_request;

QN_SDK qn_http_request_ptr qn_http_req_create(void)
{
    qn_http_request_ptr new_req = NULL;

    new_req = calloc(1, sizeof(qn_http_request));
    if (!new_req) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_req->hdr = qn_http_hdr_create();
    if (!new_req->hdr) {
        free(new_req);
        return NULL;
    } // if
    return new_req;
}

QN_SDK void qn_http_req_destroy(qn_http_request_ptr restrict req)
{
    if (req) {
        if (req->flags & QN_HTTP_REQ_USING_LOCAL_FORM) qn_http_form_destroy(req->form);
        qn_http_hdr_destroy(req->hdr);
        free(req);
    } // if
}

QN_SDK void qn_http_req_reset(qn_http_request_ptr restrict req)
{
    if (req->flags & QN_HTTP_REQ_USING_LOCAL_FORM) {
        qn_http_form_destroy(req->form);
    } // if

    qn_http_hdr_reset(req->hdr);

    req->flags = 0;
    req->body_data = NULL;
    req->body_size = 0;
    req->body_rdr = NULL;
    req->body_rdr_cb = NULL;
    req->form = NULL;
}

// ----

QN_SDK const char * qn_http_req_get_header(qn_http_request_ptr restrict req, const char * restrict hdr)
{
    return qn_http_hdr_get_value(req->hdr, hdr);
}

QN_SDK qn_bool qn_http_req_set_header_with_values(qn_http_request_ptr restrict req, const char * restrict hdr, const char * restrict val1, const char * restrict val2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string new_val = NULL;

    va_start(ap, val2);
    new_val = qn_cs_join_va("; ", val1, val2, ap);
    va_end(ap);
    if (!new_val) return qn_false;

    ret = qn_http_hdr_set_string(req->hdr, hdr, qn_str_cstr(new_val));
    qn_str_destroy(new_val);
    return ret;
}

QN_SDK qn_bool qn_http_req_set_header(qn_http_request_ptr restrict req, const char * restrict hdr, const char * restrict val)
{
    return qn_http_hdr_set_text(req->hdr, hdr, val, posix_strlen(val));
}

QN_SDK void qn_http_req_unset_header(qn_http_request_ptr restrict req, const char * restrict hdr)
{
    qn_http_hdr_unset(req->hdr, hdr);
}

// ----

QN_SDK qn_http_form_ptr qn_http_req_prepare_form(qn_http_request_ptr restrict req)
{
    if (req->form) qn_http_form_destroy(req->form);
    req->flags |= QN_HTTP_REQ_USING_LOCAL_FORM;
    return (req->form = qn_http_form_create());
}

QN_SDK qn_http_form_ptr qn_http_req_get_form(qn_http_request_ptr restrict req)
{
    return req->form;
}

QN_SDK void qn_http_req_set_form(qn_http_request_ptr restrict req, qn_http_form_ptr restrict form)
{
    req->flags &= ~QN_HTTP_REQ_USING_LOCAL_FORM;
    req->form = form;
}

// ----

QN_SDK void qn_http_req_set_body_data(qn_http_request_ptr restrict req, const char * restrict body_data, qn_size body_size)
{
    req->body_data = body_data;
    req->body_size = body_size;
}

QN_SDK void qn_http_req_set_body_reader(qn_http_request_ptr restrict req, void * restrict body_rdr, qn_http_body_reader_callback_fn body_rdr_cb, qn_fsize body_size)
{
    req->body_rdr = body_rdr;
    req->body_rdr_cb = body_rdr_cb;
    req->body_size = body_size;
}

QN_SDK const char * qn_http_req_body_data(qn_http_request_ptr restrict req)
{
    return req->body_data;
}

QN_SDK qn_fsize qn_http_req_body_size(qn_http_request_ptr restrict req)
{
    return req->body_size;
}

// ---- Definition of HTTP response ----

enum
{
    QN_HTTP_RESP_WRT_PARSING_BODY = 0,
    QN_HTTP_RESP_WRT_PARSING_DONE,
    QN_HTTP_RESP_WRT_PARSING_ERROR
};

typedef struct _QN_HTTP_RESPONSE
{
    int body_wrt_sts;
    int body_wrt_code;
    void * body_wrt;
    qn_http_data_writer_callback_fn body_wrt_cb;

    int http_code;
    qn_string http_ver;
    qn_string http_msg;

    qn_http_header_ptr hdr;
} qn_http_response;

QN_SDK qn_http_response_ptr qn_http_resp_create(void)
{
    qn_http_response_ptr new_resp = NULL;

    new_resp = calloc(1, sizeof(qn_http_response));
    if (!new_resp) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_resp->hdr = qn_http_hdr_create();
    if (!new_resp->hdr) {
        free(new_resp);
        return NULL;
    } // if

    return new_resp;
}

QN_SDK void qn_http_resp_destroy(qn_http_response_ptr restrict resp)
{
    if (resp) {
        qn_str_destroy(resp->http_ver);
        qn_str_destroy(resp->http_msg);
        qn_http_hdr_destroy(resp->hdr);
        free(resp);
    } // if
}

QN_SDK void qn_http_resp_reset(qn_http_response_ptr restrict resp)
{
    qn_str_destroy(resp->http_ver);
    resp->http_ver = NULL;
    qn_str_destroy(resp->http_msg);
    resp->http_msg = NULL;

    resp->body_wrt_sts = QN_HTTP_RESP_WRT_PARSING_BODY;
    resp->http_code = 0;
    resp->body_wrt = NULL;
    resp->body_wrt_cb = NULL;

    qn_http_hdr_reset(resp->hdr);
}

QN_SDK int qn_http_resp_get_code(qn_http_response_ptr restrict resp)
{
    return resp->http_code;
}

QN_SDK int qn_http_resp_get_writer_retcode(qn_http_response_ptr restrict resp)
{
    return resp->body_wrt_code;
}

QN_SDK qn_http_hdr_iterator_ptr qn_http_resp_get_header_iterator(qn_http_response_ptr restrict resp)
{
    return qn_http_hdr_itr_create(resp->hdr);
}

QN_SDK const char * qn_http_resp_get_header(qn_http_response_ptr restrict resp, const char * restrict hdr)
{
    return qn_http_hdr_get_value(resp->hdr, hdr);
}

QN_SDK qn_bool qn_http_resp_set_header(qn_http_response_ptr restrict resp, const char * restrict hdr, const char * restrict val, qn_size val_size)
{
    return qn_http_hdr_set_text(resp->hdr, hdr, val, val_size);
}

QN_SDK void qn_http_resp_unset_header(qn_http_response_ptr restrict resp, const char * restrict hdr)
{
    qn_http_hdr_unset(resp->hdr, hdr);
}

QN_SDK void qn_http_resp_set_data_writer(qn_http_response_ptr restrict resp, void * restrict body_wrt, qn_http_data_writer_callback_fn body_wrt_cb)
{
    resp->body_wrt = body_wrt;
    resp->body_wrt_cb = body_wrt_cb;
}

static size_t qn_http_resp_hdr_wrt_write_cfn(char * buf, size_t size, size_t nitems, void * user_data)
{
    qn_http_response_ptr resp = (qn_http_response_ptr) user_data;
    size_t buf_size = size * nitems;
    int i;
    char * begin;
    char * end;
    char * val_begin;
    char * val_end;

    if (resp->http_code == 0) {
        // Parse response status line.
        begin = strchr(buf, '/');

        // ---- http version
        if (!begin) return 0;
        begin += 1;
        end = strchr(begin, ' ');
        if (!end) return 0;

        resp->http_ver = qn_cs_clone(begin, end - begin);
        if (!resp->http_ver) return 0;

        // ---- http code
        begin = end + 1;
        end = strchr(begin, ' ');
        if (!end) return 0;

        for (i = 0; i < end - begin; i += 1) {
            if (!isdigit(begin[i])) return 0;
            resp->http_code = resp->http_code * 10 + (begin[i] - '0');
        } // for

        // ---- http message
        begin = end + 1;
        end = buf + buf_size;
        if (end[-1] != '\n') return 0;
        end -= (end[-2] == '\r') ? 2 : 1;

        resp->http_msg = qn_cs_clone(begin, end - begin);
        if (!resp->http_msg) return 0;
    } else {
        // Parse response headers.
        begin = buf;
        end = strchr(begin, ':');

        if (!end) {
            if ((begin[0] == '\r' && begin[1] == '\n') || begin[0] == '\n') return buf_size;
            return 0;
        } // if

        while (isspace(begin[0])) begin += 1;
        while (isspace(end[-1])) end -= 1;

        val_begin = end + 1;
        val_end = buf + buf_size;
        val_end -= (val_end[-2] == '\r') ? 2 : 1;
        while (isspace(val_begin[0])) val_begin += 1;
        while (isspace(val_end[-1])) val_end -= 1;

        if (!qn_http_hdr_set_raw(resp->hdr, begin, end - begin, val_begin, val_end - val_begin)) return 0;
    } // if
    return buf_size;
}

static size_t qn_http_resp_body_wrt_write_cfn(char * buf, size_t size, size_t nitems, void * user_data)
{
    qn_http_response_ptr resp = (qn_http_response_ptr) user_data;
    size_t buf_size = size * nitems;

    // **NOTE**: If the writing is done, or encounter an error, always return the buf_size for consuming all data received.
    switch (resp->body_wrt_sts) {
        case QN_HTTP_RESP_WRT_PARSING_BODY:
            resp->body_wrt_code = resp->body_wrt_cb(resp->body_wrt, buf, buf_size);
            if (resp->body_wrt_code == 0) {
                if (qn_err_json_is_need_more_text_input()) return buf_size;
                resp->body_wrt_sts = QN_HTTP_RESP_WRT_PARSING_ERROR;
            } else if (qn_err_is_succeed()) {
                resp->body_wrt_sts = QN_HTTP_RESP_WRT_PARSING_DONE;
            } // if
            return buf_size;
        
        case QN_HTTP_RESP_WRT_PARSING_DONE:
        case QN_HTTP_RESP_WRT_PARSING_ERROR:
            return buf_size;
    } // if
    return 0;
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

QN_SDK qn_http_connection_ptr qn_http_conn_create(void)
{
    qn_http_connection_ptr new_conn = NULL;

    new_conn = calloc(1, sizeof(qn_http_connection));
    if (!new_conn) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    new_conn->curl = curl_easy_init();
    if (!new_conn->curl) {
        free(new_conn);
        qn_err_3rdp_set_curl_easy_error_occurred(CURLE_FAILED_INIT);
        return NULL;
    }
    curl_easy_setopt(new_conn->curl, CURLOPT_NOSIGNAL, 1L);
    return new_conn;
}

QN_SDK void qn_http_conn_destroy(qn_http_connection_ptr restrict conn)
{
    if (conn) {
        curl_easy_cleanup(conn->curl);
        free(conn);
    } // if
}

static size_t qn_http_conn_body_reader(char * ptr, size_t size, size_t nmemb, void * user_data)
{
    qn_http_request_ptr req = (qn_http_request_ptr) user_data;
    return req->body_rdr_cb(req->body_rdr, ptr, size * nmemb);
}

static qn_bool qn_http_conn_do_request(qn_http_connection_ptr restrict conn, qn_http_request_ptr restrict req, qn_http_response_ptr restrict resp)
{
    CURLcode curl_code;
    struct curl_slist * headers = NULL;
    struct curl_slist * headers2 = NULL;
    qn_string entry = NULL;
    qn_http_hdr_iterator_ptr itr;

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_HEADERFUNCTION, qn_http_resp_hdr_wrt_write_cfn)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } // if
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_HEADERDATA, resp)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } // if

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION, qn_http_resp_body_wrt_write_cfn)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } // if
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, resp)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } // if

    if (qn_http_hdr_count(req->hdr) > 0) {
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

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_HTTPHEADER, headers)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } // if

    curl_code = curl_easy_perform(conn->curl);
    curl_slist_free_all(headers);
    if (curl_code != CURLE_OK) {
        switch (curl_code) {
            case CURLE_COULDNT_CONNECT:
            case CURLE_OPERATION_TIMEDOUT:
                qn_err_set_try_again();
                return qn_false;

            case CURLE_COULDNT_RESOLVE_HOST:
            case CURLE_COULDNT_RESOLVE_PROXY:
                qn_err_comm_set_dns_failed();
                return qn_false;

            case CURLE_SEND_ERROR:
            case CURLE_RECV_ERROR:
                qn_err_comm_set_transmission_failed();
                return qn_false;

            // case CURLE_HTTP2:
            // case CURLE_HTTP2_STREAM:
            // case CURLE_SSL_CONNECT_ERROR:
            case CURLE_PARTIAL_FILE:
                qn_err_http_set_mismatching_file_size();
                return qn_false;

            default:
                break;
        } // switch
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_http_conn_get(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_request_ptr restrict req, qn_http_response_ptr restrict resp)
{
    CURLcode curl_code;

    curl_easy_reset(conn->curl);
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POST, 0)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } // if
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_URL, url)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } // if
    return qn_http_conn_do_request(conn, req, resp);
}

QN_SDK qn_bool qn_http_conn_post(qn_http_connection_ptr restrict conn, const char * restrict url, qn_http_request_ptr restrict req, qn_http_response_ptr restrict resp)
{
    CURLcode curl_code;

    curl_easy_reset(conn->curl);

    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POST, 1)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } // if
    if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_URL, url)) != CURLE_OK) {
        qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
        return qn_false;
    } // if

    if (req->form) {
        if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_HTTPPOST, req->form->first)) != CURLE_OK) {
            qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
            return qn_false;
        } // if
        
        if (req->body_rdr && req->body_rdr_cb) {
            if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_READFUNCTION, qn_http_conn_body_reader)) != CURLE_OK) {
                qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
                return qn_false;
            } // if
        } // if
    } else if (req->body_data) {
        if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POSTFIELDS, req->body_data)) != CURLE_OK) {
            qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
            return qn_false;
        } // if
        if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_POSTFIELDSIZE, req->body_size)) != CURLE_OK) {
            qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
            return qn_false;
        } // if
    } else {
        if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_READFUNCTION, qn_http_conn_body_reader)) != CURLE_OK) {
            qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
            return qn_false;
        } // if
        if ((curl_code = curl_easy_setopt(conn->curl, CURLOPT_READDATA, req)) != CURLE_OK) {
            qn_err_3rdp_set_curl_easy_error_occurred(curl_code);
            return qn_false;
        } // if
    } // form

    return qn_http_conn_do_request(conn, req, resp);
}

#ifdef __cplusplus
}
#endif

