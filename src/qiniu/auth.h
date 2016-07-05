#ifndef __QN_AUTH_H__
#define __QN_AUTH_H__

#include "qiniu/base/basic_types.h"
#include "qiniu/base/string.h"
#include "qiniu/base/json.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct _QN_MAC;
typedef struct _QN_MAC * qn_mac_ptr;

// ---- Put Policy ----

extern qn_json_ptr qn_pp_create(const qn_string bucket, const qn_string key, qn_uint32 deadline);
extern void qn_pp_destroy(qn_json_ptr pp);

extern qn_bool qn_pp_set_scope(qn_json_ptr pp, const qn_string bucket, const qn_string key);
extern qn_bool qn_pp_set_deadline(qn_json_ptr pp, qn_uint32 deadline);

extern qn_bool qn_pp_dont_overwrite(qn_json_ptr pp);

extern qn_bool qn_pp_return_to(qn_json_ptr pp, const qn_string url, const qn_string body);
extern qn_bool qn_pp_callback_to(qn_json_ptr pp, const qn_string url, const qn_string host_name);
extern qn_bool qn_pp_callback_with_body(qn_json_ptr pp, const qn_string body, const qn_string mime_type);

extern qn_bool qn_pp_pfop_set_commands(qn_json_ptr pp, const qn_string pipeline, const qn_string cmd1, const qn_string cmd2, ...);
extern qn_bool qn_pp_pfop_set_command_list(qn_json_ptr pp, const qn_string pipeline, const qn_string cmds[], int cmd_count);
extern qn_bool qn_pp_pfop_notify_to(qn_json_ptr pp, const qn_string mime_type);

extern qn_bool qn_pp_mime_enable_auto_detecting(qn_json_ptr pp);
extern qn_bool qn_pp_mime_allow(qn_json_ptr pp, const qn_string mime1, const qn_string mime2, ...);
extern qn_bool qn_pp_mime_allow_list(qn_json_ptr pp, const qn_string mime[], int mime_count);
extern qn_bool qn_pp_mime_deny(qn_json_ptr pp, const qn_string mime1, const qn_string mime2, ...);
extern qn_bool qn_pp_mime_deny_list(qn_json_ptr pp, const qn_string mime[], int mime_count);

extern qn_bool qn_pp_fsize_set_minimum(qn_json_ptr pp, qn_uint32 min_size);
extern qn_bool qn_pp_fsize_set_maximum(qn_json_ptr pp, qn_uint32 max_size);

extern qn_bool qn_pp_key_enable_fetching_from_callback_response(qn_json_ptr pp);
extern qn_bool qn_pp_key_make_from_template(qn_json_ptr pp, const qn_string key_template);

extern qn_bool qn_pp_auto_delete_after_days(qn_json_ptr pp, qn_uint32 days);

extern qn_string qn_pp_to_uptoken(qn_json_ptr pp, qn_mac_ptr mac);

// ---- Authorization Functions ----

extern qn_mac_ptr qn_mac_create(const char * access_key, const char * secret_key);
extern void qn_mac_destroy(qn_mac_ptr mac);

extern qn_string qn_mac_make_uptoken(qn_mac_ptr mac, const char * data, qn_size data_size);
extern qn_string qn_mac_make_dnurl(qn_mac_ptr mac, const qn_string url, qn_uint32 deadline);

#ifdef __cplusplus
}
#endif

#endif // __QN_AUTH_H__

