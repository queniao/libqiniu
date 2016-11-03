#ifndef __QN_JSON_PARSER_H__
#define __QN_JSON_PARSER_H__

#include "qiniu/base/json.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of parser of JSON ----

struct _QN_JSON_PARSER;
typedef struct _QN_JSON_PARSER * qn_json_parser_ptr;

QN_API extern qn_json_parser_ptr qn_json_prs_create(void);
QN_API extern void qn_json_prs_destroy(qn_json_parser_ptr restrict prs);
QN_API extern qn_bool qn_json_prs_parse_object(qn_json_parser_ptr restrict prs, const char * restrict buf, size_t * restrict buf_size, qn_json_object_ptr * restrict root);
QN_API extern qn_bool qn_json_prs_parse_array(qn_json_parser_ptr restrict prs, const char * restrict buf, size_t * restrict buf_size, qn_json_array_ptr * restrict root);

QN_API extern void qn_json_prs_set_max_levels(int count);

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_PARSER_H__

