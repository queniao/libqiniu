#include "auth.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Put Policy ----

qn_json_ptr qn_pp_create(const qn_string_ptr bucket, const qn_string_ptr key, qn_uint32 deadline)
{
    qn_json_ptr pp = qn_json_create_object();

    if (pp) {
        if (!qn_pp_set_scope(pp, bucket, key)) {
            return NULL;
        } // if
        if (!qn_pp_set_deadline(pp, deadline)) {
            return NULL;
        } // if
    } // if
    return pp;
} // qn_pp_create

void qn_pp_destroy(qn_json_ptr pp)
{
    qn_json_destroy(pp);
} // qn_pp_destroy

qn_bool qn_pp_set_scope(qn_json_ptr pp, const qn_string_ptr bucket, const qn_string_ptr key)
{
    qn_string_ptr scope = NULL;
    if (key) {
        scope = qn_str_sprintf("%s:%s", qn_str_cstr(bucket), qn_str_cstr(key));
        if (!scope) {
            return qn_false;
        } // if
    } else {
        scope = bucket;
    } // if
    return qn_json_set_string(pp, "scope", scope);
} // qn_pp_set_scope

qn_bool qn_pp_set_deadline(qn_json_ptr pp, qn_uint32 deadline)
{
    return qn_json_set_integer(pp, "deadline", deadline);
} // qn_pp_set_deadline

qn_bool qn_pp_dont_overwrite(qn_json_ptr pp)
{
    qn_json_ptr mode = qn_json_create_integer(1);
    if (!mode) {
        return qn_false;
    } // if
    return qn_json_set(pp, "insertOnly", mode);
} // qn_pp_dont_overwrite

qn_bool qn_pp_return_to(qn_json_ptr pp, const qn_string_ptr url, const qn_string_ptr body)
{
    if (!qn_json_set_string(pp, "returnUrl", url)) {
        return qn_false;
    } // if
    if (body) {
        return qn_json_set_string(pp, "returnBody", body);
    } // if
    return qn_true;
} // qn_pp_return_to

qn_bool qn_pp_callback_to(qn_json_ptr pp, const qn_string_ptr url, const qn_string_ptr host_name)
{
    if (!qn_json_set_string(pp, "callbackUrl", url)) {
        return qn_false;
    } // if
    if (host_name) {
        return qn_json_set_string(pp, "callbackHost", host_name);
    } // if
    return qn_true;
} // qn_pp_callback_to

qn_bool qn_pp_callback_with_body(qn_json_ptr pp, const qn_string_ptr body, const qn_string_ptr mime_type)
{
    if (!qn_json_set_string(pp, "callbackBody", body)) {
        return qn_false;
    } // if
    if (mime_type) {
        return qn_json_set_string(pp, "callbackBodyType", mime_type);
    } // if
    return qn_true;
} // qn_pp_callback_with_body

qn_bool qn_pp_pfop_set_commands(qn_json_ptr pp, const qn_string_ptr pipeline, const qn_string_ptr cmd1, const qn_string_ptr cmd2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string_ptr ops = NULL;

    if (!cmd2) {
        // Only one command passed.
        ret = qn_json_set_string(pp, "persistentOps", cmd1);
    } else {
        va_start(ap, cmd2);
        ops = qn_str_vjoin(";", cmd1, cmd2, ap);
        va_end(ap);
        if (!ops) return qn_false;
        ret = qn_json_set_string(pp, "persistentOps", ops);
        qn_str_destroy(ops);
    } // if

    if (ret && pipeline && qn_str_size(pipeline) > 0) {
        ret = qn_json_set_string(pp, "persistentPipeline", pipeline);
    } // if
    return ret;
} // qn_pp_pfop_set_commands

qn_bool qn_pp_pfop_set_command_list(qn_json_ptr pp, const qn_string_ptr pipeline, const qn_string_ptr cmds[], int cmd_count)
{
    qn_bool ret = qn_false;
    qn_string_ptr ops = NULL;

    if (cmd_count == 1) {
        // Only one command passed.
        ret = qn_json_set_string(pp, "persistentOps", cmds[0]);
    } else {
        ops = qn_str_ajoin(";", cmds, cmd_count);
        if (!ops) return qn_false;
        ret = qn_json_set_string(pp, "persistentOps", ops);
        qn_str_destroy(ops);
    } // if
    
    if (ret && pipeline && qn_str_size(pipeline) > 0) {
        ret = qn_json_set_string(pp, "persistentPipeline", pipeline);
    } // if
    return ret;
} // qn_pp_pfop_set_command_list

qn_bool qn_pp_pfop_notify_to(qn_json_ptr pp, const qn_string_ptr mime_type)
{
    return qn_json_set_string(pp, "persistentNotifyUrl", mime_type);
} // qn_pp_pfop_notify_to

qn_bool qn_pp_mime_enable_auto_detecting(qn_json_ptr pp)
{
    return qn_json_set_integer(pp, "detectMime", 1);
} // qn_pp_mime_enable_auto_detecting

qn_bool qn_pp_mime_allow(qn_json_ptr pp, const qn_string_ptr mime1, const qn_string_ptr mime2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string_ptr mime_str = NULL;

    if (!mime2) {
        // Only one mime passed.
        return qn_json_set_string(pp, "mimeLimit", mime1);
    } // if

    va_start(ap, mime2);
    mime_str = qn_str_vjoin(";", mime1, mime2, ap);
    va_end(ap);
    if (!mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", mime_str);
    qn_str_destroy(mime_str);
    return ret;
} // qn_pp_mime_allow

qn_bool qn_pp_mime_allow_list(qn_json_ptr pp, const qn_string_ptr mime_list[], int mime_count)
{
    qn_bool ret = qn_false;
    qn_string_ptr mime_str = NULL;
    
    if (mime_count == 1) {
        return qn_json_set_string(pp, "mimeLimit", mime_list[0]);
    } // if

    mime_str = qn_str_ajoin(";", mime_list, mime_count);
    if (!mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", mime_str);
    qn_str_destroy(mime_str);
    return ret;
} // qn_pp_mime_allow_list

qn_bool qn_pp_mime_deny(qn_json_ptr pp, const qn_string_ptr mime1, const qn_string_ptr mime2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string_ptr deny_mime_str = NULL;
    qn_string_ptr mime_str = NULL;

    if (!mime2) {
        // Only one mime passed.
        return qn_json_set_string(pp, "mimeLimit", mime1);
    } // if

    va_start(ap, mime2);
    mime_str = qn_str_vjoin(";", mime1, mime2, ap);
    va_end(ap);
    if (!mime_str) return qn_false;

    deny_mime_str = qn_str_sprintf("!%*s", qn_str_size(mime_str), qn_str_cstr(mime_str));
    qn_str_destroy(mime_str);
    if (!deny_mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", deny_mime_str);
    qn_str_destroy(deny_mime_str);
    return ret;
} // qn_pp_mime_deny

qn_bool qn_pp_mime_deny_list(qn_json_ptr pp, const qn_string_ptr mime_list[], int mime_count)
{
    qn_bool ret = qn_false;
    qn_string_ptr deny_mime_str = NULL;
    qn_string_ptr mime_str = NULL;
    
    if (mime_count == 1) { 
        deny_mime_str = qn_str_sprintf("!%*s", qn_str_size(mime_list[0]), qn_str_cstr(mime_list[0]));
    } else {
        qn_str_ajoin(";", mime_list, mime_count);
        if (!mime_str) return qn_false;

        deny_mime_str = qn_str_sprintf("!%*s", qn_str_size(mime_str), qn_str_cstr(mime_str));
        qn_str_destroy(mime_str);
    } // if
    if (!deny_mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", deny_mime_str);
    qn_str_destroy(deny_mime_str);
    return ret;
} // qn_pp_mime_deny_list

qn_bool qn_pp_fsize_set_minimum(qn_json_ptr pp, qn_uint32 min_size)
{
    return qn_json_set_integer(pp, "fsizeMin", min_size);
} // qn_pp_fsize_set_minimum

qn_bool qn_pp_fsize_set_maximum(qn_json_ptr pp, qn_uint32 max_size)
{
    return qn_json_set_integer(pp, "fsizeLimit", max_size);
} // qn_pp_fsize_set_maximum

qn_bool qn_pp_key_enable_fetching_from_callback_response(qn_json_ptr pp)
{
    return qn_json_set_integer(pp, "callbackFetchKey", 1);
} // qn_pp_key_enable_fetching_from_callback_response

qn_bool qn_pp_key_make_from_template(qn_json_ptr pp, const qn_string_ptr key_template)
{
    return qn_json_set_string(pp, "saveKey", key_template);
} // qn_pp_key_make_from_template

qn_bool qn_pp_auto_delete_after_days(qn_json_ptr pp, qn_uint32 days)
{
    return qn_json_set_integer(pp, "deleteAfterDays", days);
} // qn_pp_auto_delete_after_days

// ---- Authorization ----

qn_string_ptr qn_auth_make_uptoken(qn_mac_ptr mac, qn_json_ptr put_policy);
qn_string_ptr qn_auth_make_dnurl(qn_mac_ptr mac, const qn_string_ptr url, qn_uint32 deadline);

#ifdef __cplusplus
}
#endif

