#ifndef __QN_JSON_FORMATTER_H__
#define __QN_JSON_FORMATTER_H__

#include "qiniu/base/json.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of formatter of JSON ----

struct _QN_JSON_FORMATTER;
typedef struct _QN_JSON_FORMATTER * qn_json_formatter_ptr;

QN_API extern qn_json_formatter_ptr qn_json_fmt_create(void);
QN_API extern void qn_json_fmt_destroy(qn_json_formatter_ptr restrict fmt);

QN_API extern void qn_json_fmt_enable_escape_utf8_string(qn_json_formatter_ptr restrict fmt);
QN_API extern void qn_json_fmt_disable_escape_utf8_string(qn_json_formatter_ptr restrict fmt);

QN_API extern qn_bool qn_json_fmt_format_object(qn_json_formatter_ptr restrict fmt, qn_json_object_ptr restrict root, char * restrict buf, qn_size * restrict buf_size);
QN_API extern qn_bool qn_json_fmt_format_array(qn_json_formatter_ptr restrict fmt, qn_json_array_ptr restrict root, char * restrict buf, qn_size * restrict buf_size);

QN_API extern qn_string qn_json_object_to_string(qn_json_object_ptr restrict root);
QN_API extern qn_string qn_json_array_to_string(qn_json_array_ptr restrict root);

#ifdef __cplusplus
}
#endif

#endif // __QN_JSON_FORMATTER_H__

