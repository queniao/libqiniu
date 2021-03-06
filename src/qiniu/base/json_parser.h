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

QN_SDK extern qn_json_parser_ptr qn_json_prs_create(void);
QN_SDK extern void qn_json_prs_destroy(qn_json_parser_ptr restrict prs);
QN_SDK extern qn_bool qn_json_prs_parse_object(qn_json_parser_ptr restrict prs, const char * restrict buf, qn_size * restrict buf_size, qn_json_object_ptr * restrict root);
QN_SDK extern qn_bool qn_json_prs_parse_array(qn_json_parser_ptr restrict prs, const char * restrict buf, qn_size * restrict buf_size, qn_json_array_ptr * restrict root);

QN_SDK extern qn_json_object_ptr qn_json_object_from_string(const char * restrict buf, qn_size buf_size);
QN_SDK extern qn_json_array_ptr qn_json_array_from_string(const char * restrict buf, qn_size buf_size);

QN_SDK extern void qn_json_prs_set_max_levels(int count);

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_PARSER_H__

