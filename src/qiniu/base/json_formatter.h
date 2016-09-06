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

extern void qn_json_fmt_enable_escape_utf8_string(qn_json_formatter_ptr fmt);
extern void qn_json_fmt_disable_escape_utf8_string(qn_json_formatter_ptr fmt);

extern qn_bool qn_json_fmt_format_object(qn_json_formatter_ptr fmt, qn_json_object_ptr root, char * restrict buf, qn_size * restrict buf_size);
extern qn_bool qn_json_fmt_format_array(qn_json_formatter_ptr fmt, qn_json_array_ptr root, char * restrict buf, qn_size * restrict buf_size);

extern qn_string qn_json_object_to_string(qn_json_object_ptr root);
extern qn_string qn_json_array_to_string(qn_json_array_ptr root);

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_FORMATTER_H__

