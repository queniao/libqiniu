#ifndef __QN_JSON_PARSER_H__
#define __QN_JSON_PARSER_H__

#include "qiniu/base/json.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of parser of JSON ----

struct _QN_JSON_PARSER;
typedef struct _QN_JSON_PARSER * qn_json_parser_ptr;

extern qn_json_parser_ptr qn_json_prs_create(void);
extern void qn_json_prs_destroy(qn_json_parser_ptr prs);
extern void qn_json_prs_reset(qn_json_parser_ptr prs);
extern qn_bool qn_json_prs_parse(qn_json_parser_ptr prs, const char * restrict buf, qn_size * restrict buf_size, qn_json_ptr * root_element);

extern void qn_json_prs_set_max_levels(qn_size count);

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_PARSER_H__

