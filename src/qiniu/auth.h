#ifndef __QN_AUTH_H__
#define __QN_AUTH_H__

#include "qiniu/base/basic_types.h"
#include "qiniu/base/string.h"
#include "qiniu/base/json.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct _QN_MAC;
typedef struct _QN_MAC * qn_mac_ptr;

// ---- Declaration of Put Policy ----

QN_API extern qn_json_object_ptr qn_pp_create(const char * restrict bucket, const char * restrict key, qn_uint32 deadline);
QN_API extern void qn_pp_destroy(qn_json_object_ptr restrict pp);

QN_API extern qn_bool qn_pp_set_scope(qn_json_object_ptr restrict pp, const char * restrict bucket, const char * restrict key);
QN_API extern qn_bool qn_pp_set_deadline(qn_json_object_ptr restrict pp, qn_uint32 deadline);

QN_API extern qn_bool qn_pp_dont_overwrite(qn_json_object_ptr restrict pp);

QN_API extern qn_bool qn_pp_return_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict body);
QN_API extern qn_bool qn_pp_return_to_client(qn_json_object_ptr restrict pp, const char * restrict body);

QN_API extern qn_bool qn_pp_callback_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict host_name);
QN_API extern qn_bool qn_pp_callback_with_body(qn_json_object_ptr restrict pp, const char * restrict body, const char * restrict mime_type);

QN_API extern qn_bool qn_pp_pfop_set_commands(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char * restrict cmd1, const char * restrict cmd2, ...);
QN_API extern qn_bool qn_pp_pfop_set_command_list(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char ** restrict cmds, int cmd_count);
QN_API extern qn_bool qn_pp_pfop_notify_to_server(qn_json_object_ptr restrict pp, const char * restrict url);

QN_API extern qn_bool qn_pp_mime_enable_auto_detecting(qn_json_object_ptr restrict pp);
QN_API extern qn_bool qn_pp_mime_allow(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...);
QN_API extern qn_bool qn_pp_mime_allow_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, int mime_count);
QN_API extern qn_bool qn_pp_mime_deny(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...);
QN_API extern qn_bool qn_pp_mime_deny_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, int mime_count);

QN_API extern qn_bool qn_pp_fsize_set_minimum(qn_json_object_ptr restrict pp, qn_uint32 min_size);
QN_API extern qn_bool qn_pp_fsize_set_maximum(qn_json_object_ptr restrict pp, qn_uint32 max_size);

QN_API extern qn_bool qn_pp_key_enable_fetching_from_callback_response(qn_json_object_ptr restrict pp);
QN_API extern qn_bool qn_pp_key_make_from_template(qn_json_object_ptr restrict pp, const char * restrict key_template);

QN_API extern qn_bool qn_pp_auto_delete_after_days(qn_json_object_ptr restrict pp, qn_uint32 days);

QN_API extern qn_string qn_pp_to_uptoken(qn_json_object_ptr restrict pp, qn_mac_ptr restrict mac);

// ---- Declaration of Authorization ----

QN_API extern qn_mac_ptr qn_mac_create(const char * restrict access_key, const char * restrict secret_key);
QN_API extern void qn_mac_destroy(qn_mac_ptr restrict mac);

QN_API extern const qn_string qn_mac_make_uptoken(qn_mac_ptr restrict mac, const char * restrict pp, size_t pp_size);
QN_API extern const qn_string qn_mac_make_acctoken(qn_mac_ptr restrict mac, const char * restrict url, const char * restrict body, size_t body_size);
QN_API extern const qn_string qn_mac_make_dnurl(qn_mac_ptr restrict mac, const char * restrict url, qn_uint32 deadline);

#ifdef __cplusplus
}
#endif

#endif // __QN_AUTH_H__

