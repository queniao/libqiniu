#ifndef __QN_HTTP_HEADER_PARSER_H__
#define __QN_HTTP_HEADER_PARSER_H__

#include "qiniu/http_header.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of HTTP header parser ----

struct _QN_HTTP_HEADER_PARSER;
typedef struct _QN_HTTP_HEADER_PARSER * qn_http_hdr_parser_ptr;

extern qn_http_hdr_parser_ptr qn_http_hdr_prs_create(void);
extern void qn_http_hdr_prs_destroy(qn_http_hdr_parser_ptr prs);
extern void qn_http_hdr_prs_reset(qn_http_hdr_parser_ptr prs);

extern qn_bool qn_http_hdr_prs_parse(qn_http_hdr_parser_ptr prs, const char * buf, int * buf_size, qn_http_header_ptr * hdr);

#ifdef __cplusplus
}
#endif

// ---- Declaration of HTTP header ----

#endif // __QN_HTTP_HEADER_PARSER_H__

