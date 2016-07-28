#ifndef __QN_HTTP_H__
#define __QN_HTTP_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*qn_http_body_reader)(void * reader_data, char * buf, int size);
typedef int (*qn_http_body_writer)(void * writer_data, char * buf, int size);

// ---- Declaration of HTTP request ----

struct _QN_HTTP_REQUEST;
typedef struct _QN_HTTP_REQUEST * qn_http_request_ptr;

extern qn_http_request_ptr qn_http_req_create(void);
extern void qn_http_req_destroy(qn_http_request_ptr req);
extern void qn_http_req_reset(qn_http_request_ptr req);

extern qn_string qn_http_req_get_header(qn_http_request_ptr req, const qn_string header);
extern qn_bool qn_http_req_set_header_with_values(qn_http_request_ptr req, const qn_string header, const qn_string val1, const qn_string val2, ...);
extern qn_bool qn_http_req_set_header(qn_http_request_ptr req, const qn_string header, const qn_string value);
extern void qn_http_req_unset_header(qn_http_request_ptr req, const qn_string header);

extern void qn_http_req_set_body_reader(qn_http_request_ptr req, void * body_reader_data, qn_http_body_reader body_reader, qn_size body_size);

// ---- Declaration of HTTP response ----

struct _QN_HTTP_RESPONSE;
typedef struct _QN_HTTP_RESPONSE * qn_http_response_ptr;

extern qn_http_response_ptr qn_http_resp_create(void);
extern void qn_http_resp_destroy(qn_http_response_ptr resp);
extern void qn_http_resp_reset(qn_http_response_ptr resp);

extern int qn_http_resp_get_code(qn_http_response_ptr resp);
extern int qn_http_resp_get_writer_retcode(qn_http_response_ptr resp);

extern qn_string qn_http_resp_get_header(qn_http_response_ptr resp, const qn_string header);
extern qn_bool qn_http_resp_set_header(qn_http_response_ptr resp, const qn_string header, const qn_string value);
extern void qn_http_resp_unset_header(qn_http_response_ptr resp, const qn_string header);

extern void qn_http_resp_set_body_writer(qn_http_response_ptr resp, void * writer_data, qn_http_body_writer writer);

// ---- Declaration of HTTP connection ----

struct _QN_HTTP_CONNECTION;
typedef struct _QN_HTTP_CONNECTION * qn_http_connection_ptr;

extern qn_http_connection_ptr qn_http_conn_create(const qn_string ip, int port);
extern void qn_http_conn_destroy(qn_http_connection_ptr conn);

extern qn_bool qn_http_conn_get(qn_http_connection_ptr conn, const qn_string url, qn_http_request_ptr req, qn_http_response_ptr resp);
extern qn_bool qn_http_conn_post(qn_http_connection_ptr conn, const qn_string url, qn_http_request_ptr req, qn_http_response_ptr resp);

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_H__

