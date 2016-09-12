#ifndef __QN_HTTP_HEADER_PARSER_H__
#define __QN_HTTP_HEADER_PARSER_H__

#include "qiniu/http_header.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of HTTP header parser ----

struct _QN_HTTP_HEADER_PARSER;
typedef struct _QN_HTTP_HEADER_PARSER * qn_http_hdr_parser_ptr;

QN_API extern qn_http_hdr_parser_ptr qn_http_hdr_prs_create(void);
QN_API extern void qn_http_hdr_prs_destroy(qn_http_hdr_parser_ptr restrict prs);
QN_API extern void qn_http_hdr_prs_reset(qn_http_hdr_parser_ptr restrict prs);

QN_API extern qn_bool qn_http_hdr_prs_parse(qn_http_hdr_parser_ptr restrict prs, const char * restrict buf, int * restrict buf_size, qn_http_header_ptr * restrict hdr);

#ifdef __cplusplus
}
#endif

// ---- Declaration of HTTP header ----

#endif // __QN_HTTP_HEADER_PARSER_H__

