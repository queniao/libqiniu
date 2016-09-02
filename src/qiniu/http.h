#ifndef __QN_HTTP_H__
#define __QN_HTTP_H__

#include "qiniu/base/string.h"
#include "qiniu/base/json.h"
#include "qiniu/http_header.h"
#include "qiniu/os/file.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of body reader and writer

typedef qn_size (*qn_http_body_reader_callback)(void * reader, char * buf, qn_size size);
typedef qn_size (*qn_http_data_writer_callback)(void * writer, char * buf, qn_size size);

// ----

struct _QN_HTTP_JSON_WRITER;
typedef struct _QN_HTTP_JSON_WRITER * qn_http_json_writer_ptr;

extern qn_http_json_writer_ptr qn_http_json_wrt_create(void);
extern void qn_http_json_wrt_destroy(qn_http_json_writer_ptr writer);

extern void qn_http_json_wrt_prepare_for_object(qn_http_json_writer_ptr writer, qn_json_object_ptr * obj);
extern void qn_http_json_wrt_prepare_for_array(qn_http_json_writer_ptr writer, qn_json_array_ptr * arr);
extern qn_size qn_http_json_wrt_callback(void * writer, char * buf, qn_size buf_size);

// ---- Declaration of HTTP form

struct _QN_HTTP_FORM;
typedef struct _QN_HTTP_FORM * qn_http_form_ptr;

extern qn_http_form_ptr qn_http_form_create(void);
extern void qn_http_form_destroy(qn_http_form_ptr form);

extern qn_bool qn_http_form_add_string(qn_http_form_ptr form, const char * restrict field, const char * restrict value, qn_size size);
extern qn_bool qn_http_form_add_file(qn_http_form_ptr form, const char * restrict field, const char * restrict fname, const char * restrict fname_utf8, qn_size fsize);
extern qn_bool qn_http_form_add_buffer(qn_http_form_ptr form, const char * restrict field, const char * restrict fname, const char * restrict buf, int buf_size);

// ---- Declaration of HTTP request ----

struct _QN_HTTP_REQUEST;
typedef struct _QN_HTTP_REQUEST * qn_http_request_ptr;

extern qn_http_request_ptr qn_http_req_create(void);
extern void qn_http_req_destroy(qn_http_request_ptr req);
extern void qn_http_req_reset(qn_http_request_ptr req);

// ----

extern qn_bool qn_http_req_get_header_raw(qn_http_request_ptr req, const char * hdr, qn_size hdr_size, const char ** val, qn_size * val_size);

static inline qn_bool qn_http_req_get_header(qn_http_request_ptr req, const qn_string hdr, const char ** val, qn_size * val_size)
{
    return qn_http_req_get_header_raw(req, qn_str_cstr(hdr), qn_str_size(hdr), val, val_size);
}

extern qn_bool qn_http_req_set_header_with_values(qn_http_request_ptr req, const qn_string header, const qn_string val1, const qn_string val2, ...);
extern qn_bool qn_http_req_set_header(qn_http_request_ptr req, const qn_string header, const qn_string value);
extern qn_bool qn_http_req_set_header_raw(qn_http_request_ptr req, const char * hdr, int hdr_size, const char * val, int val_size);
extern void qn_http_req_unset_header(qn_http_request_ptr req, const qn_string header);

// ----

extern qn_http_form_ptr qn_http_req_get_form(qn_http_request_ptr req);
extern void qn_http_req_set_form(qn_http_request_ptr req, qn_http_form_ptr form);

// ----

extern void qn_http_req_set_body_data(qn_http_request_ptr req, char * body_data, qn_size body_size);
extern void qn_http_req_set_body_reader(qn_http_request_ptr req, void * body_reader, qn_http_body_reader_callback body_reader_callback, qn_size body_size);

// ---- Declaration of HTTP response ----

struct _QN_HTTP_RESPONSE;
typedef struct _QN_HTTP_RESPONSE * qn_http_response_ptr;

extern qn_http_response_ptr qn_http_resp_create(void);
extern void qn_http_resp_destroy(qn_http_response_ptr resp);
extern void qn_http_resp_reset(qn_http_response_ptr resp);

// ----

extern int qn_http_resp_get_code(qn_http_response_ptr resp);
extern int qn_http_resp_get_writer_retcode(qn_http_response_ptr resp);

// ----

extern qn_http_hdr_iterator_ptr qn_http_resp_get_header_iterator(qn_http_response_ptr resp);
extern qn_bool qn_http_resp_get_header_raw(qn_http_response_ptr resp, const char * hdr, qn_size hdr_size, const char ** val, qn_size * val_size);

static inline qn_bool qn_http_resp_get_header(qn_http_response_ptr resp, const qn_string hdr, const char ** val, qn_size * val_size)
{
    return qn_http_resp_get_header_raw(resp, qn_str_cstr(hdr), qn_str_size(hdr), val, val_size);
}

extern qn_bool qn_http_resp_set_header(qn_http_response_ptr resp, const qn_string header, const qn_string value);
extern qn_bool qn_http_resp_set_header_raw(qn_http_response_ptr resp, const char * hdr, int hdr_size, const char * val, int val_size);
extern void qn_http_resp_unset_header(qn_http_response_ptr resp, const qn_string header);

// ----

extern void qn_http_resp_set_data_writer(qn_http_response_ptr resp, void * body_writer, qn_http_data_writer_callback body_writer_callback);

// ---- Declaration of HTTP connection ----

struct _QN_HTTP_CONNECTION;
typedef struct _QN_HTTP_CONNECTION * qn_http_connection_ptr;

extern qn_http_connection_ptr qn_http_conn_create(void);
extern void qn_http_conn_destroy(qn_http_connection_ptr conn);

extern qn_bool qn_http_conn_get(qn_http_connection_ptr conn, const qn_string url, qn_http_request_ptr req, qn_http_response_ptr resp);
extern qn_bool qn_http_conn_post(qn_http_connection_ptr conn, const qn_string url, qn_http_request_ptr req, qn_http_response_ptr resp);

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_H__

