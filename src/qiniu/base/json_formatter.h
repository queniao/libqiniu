#ifndef __QN_JSON_FORMATTER_H__
#define __QN_JSON_FORMATTER_H__

#include "qiniu/base/json.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of formatter of JSON ----

struct _QN_JSON_FORMATTER;
typedef struct _QN_JSON_FORMATTER * qn_json_formatter_ptr;

extern qn_json_formatter_ptr qn_json_fmt_create(void);
extern void qn_json_fmt_destroy(qn_json_formatter_ptr fmt);
extern qn_bool qn_json_fmt_format(qn_json_formatter_ptr fmt, qn_json_ptr root_element, char * restrict buf, qn_size * restrict buf_size);
extern qn_string qn_json_format_to_string(qn_json_ptr root_element);

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_FORMATTER_H__

