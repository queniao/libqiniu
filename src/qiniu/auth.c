#include <openssl/hmac.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/auth.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_MAC
{
    qn_string access_key;
    qn_string secret_key;
} qn_mac;

// ---- Put Policy ----

qn_json_object_ptr qn_pp_create(const qn_string bucket, const qn_string key, qn_uint32 deadline)
{
    qn_json_object_ptr pp = qn_json_create_object();

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

void qn_pp_destroy(qn_json_object_ptr pp)
{
    qn_json_destroy_object(pp);
} // qn_pp_destroy

qn_bool qn_pp_set_scope(qn_json_object_ptr pp, const qn_string bucket, const qn_string key)
{
    qn_string scope = NULL;
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

qn_bool qn_pp_set_deadline(qn_json_object_ptr pp, qn_uint32 deadline)
{
    return qn_json_set_integer(pp, "deadline", deadline);
} // qn_pp_set_deadline

qn_bool qn_pp_dont_overwrite(qn_json_object_ptr pp)
{
    return qn_json_set_integer(pp, "insertOnly", 1);
} // qn_pp_dont_overwrite

qn_bool qn_pp_return_to(qn_json_object_ptr pp, const qn_string url, const qn_string body)
{
    if (!qn_json_set_string(pp, "returnUrl", url)) {
        return qn_false;
    } // if
    if (body) {
        return qn_json_set_string(pp, "returnBody", body);
    } // if
    return qn_true;
} // qn_pp_return_to

qn_bool qn_pp_callback_to(qn_json_object_ptr pp, const qn_string url, const qn_string host_name)
{
    if (!qn_json_set_string(pp, "callbackUrl", url)) {
        return qn_false;
    } // if
    if (host_name) {
        return qn_json_set_string(pp, "callbackHost", host_name);
    } // if
    return qn_true;
} // qn_pp_callback_to

qn_bool qn_pp_callback_with_body(qn_json_object_ptr pp, const qn_string body, const qn_string mime_type)
{
    if (!qn_json_set_string(pp, "callbackBody", body)) {
        return qn_false;
    } // if
    if (mime_type) {
        return qn_json_set_string(pp, "callbackBodyType", mime_type);
    } // if
    return qn_true;
} // qn_pp_callback_with_body

qn_bool qn_pp_pfop_set_commands(qn_json_object_ptr pp, const qn_string pipeline, const qn_string cmd1, const qn_string cmd2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string ops = NULL;

    if (!cmd2) {
        // Only one command passed.
        ret = qn_json_set_string(pp, "persistentOps", cmd1);
    } else {
        va_start(ap, cmd2);
        ops = qn_str_join_va(";", cmd1, cmd2, ap);
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

qn_bool qn_pp_pfop_set_command_list(qn_json_object_ptr pp, const qn_string pipeline, const qn_string cmds[], int cmd_count)
{
    qn_bool ret = qn_false;
    qn_string ops = NULL;

    if (cmd_count == 1) {
        // Only one command passed.
        ret = qn_json_set_string(pp, "persistentOps", cmds[0]);
    } else {
        ops = qn_str_join_list(";", cmds, cmd_count);
        if (!ops) return qn_false;
        ret = qn_json_set_string(pp, "persistentOps", ops);
        qn_str_destroy(ops);
    } // if
    
    if (ret && pipeline && qn_str_size(pipeline) > 0) {
        ret = qn_json_set_string(pp, "persistentPipeline", pipeline);
    } // if
    return ret;
} // qn_pp_pfop_set_command_list

qn_bool qn_pp_pfop_notify_to(qn_json_object_ptr pp, const qn_string mime_type)
{
    return qn_json_set_string(pp, "persistentNotifyUrl", mime_type);
} // qn_pp_pfop_notify_to

qn_bool qn_pp_mime_enable_auto_detecting(qn_json_object_ptr pp)
{
    return qn_json_set_integer(pp, "detectMime", 1);
} // qn_pp_mime_enable_auto_detecting

qn_bool qn_pp_mime_allow(qn_json_object_ptr pp, const qn_string mime1, const qn_string mime2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string mime_str = NULL;

    if (!mime2) {
        // Only one mime passed.
        return qn_json_set_string(pp, "mimeLimit", mime1);
    } // if

    va_start(ap, mime2);
    mime_str = qn_str_join_va(";", mime1, mime2, ap);
    va_end(ap);
    if (!mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", mime_str);
    qn_str_destroy(mime_str);
    return ret;
} // qn_pp_mime_allow

qn_bool qn_pp_mime_allow_list(qn_json_object_ptr pp, const qn_string mime_list[], int mime_count)
{
    qn_bool ret = qn_false;
    qn_string mime_str = NULL;
    
    if (mime_count == 1) {
        return qn_json_set_string(pp, "mimeLimit", mime_list[0]);
    } // if

    mime_str = qn_str_join_list(";", mime_list, mime_count);
    if (!mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", mime_str);
    qn_str_destroy(mime_str);
    return ret;
} // qn_pp_mime_allow_list

qn_bool qn_pp_mime_deny(qn_json_object_ptr pp, const qn_string mime1, const qn_string mime2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string deny_mime_str = NULL;
    qn_string mime_str = NULL;

    if (!mime2) {
        // Only one mime passed.
        return qn_json_set_string(pp, "mimeLimit", mime1);
    } // if

    va_start(ap, mime2);
    mime_str = qn_str_join_va(";", mime1, mime2, ap);
    va_end(ap);
    if (!mime_str) return qn_false;

    deny_mime_str = qn_str_sprintf("!%*s", qn_str_size(mime_str), qn_str_cstr(mime_str));
    qn_str_destroy(mime_str);
    if (!deny_mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", deny_mime_str);
    qn_str_destroy(deny_mime_str);
    return ret;
} // qn_pp_mime_deny

qn_bool qn_pp_mime_deny_list(qn_json_object_ptr pp, const qn_string mime_list[], int mime_count)
{
    qn_bool ret = qn_false;
    qn_string deny_mime_str = NULL;
    qn_string mime_str = NULL;
    
    if (mime_count == 1) { 
        deny_mime_str = qn_str_sprintf("!%*s", qn_str_size(mime_list[0]), qn_str_cstr(mime_list[0]));
    } else {
        qn_str_join_list(";", mime_list, mime_count);
        if (!mime_str) return qn_false;

        deny_mime_str = qn_str_sprintf("!%*s", qn_str_size(mime_str), qn_str_cstr(mime_str));
        qn_str_destroy(mime_str);
    } // if
    if (!deny_mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", deny_mime_str);
    qn_str_destroy(deny_mime_str);
    return ret;
} // qn_pp_mime_deny_list

qn_bool qn_pp_fsize_set_minimum(qn_json_object_ptr pp, qn_uint32 min_size)
{
    return qn_json_set_integer(pp, "fsizeMin", min_size);
} // qn_pp_fsize_set_minimum

qn_bool qn_pp_fsize_set_maximum(qn_json_object_ptr pp, qn_uint32 max_size)
{
    return qn_json_set_integer(pp, "fsizeLimit", max_size);
} // qn_pp_fsize_set_maximum

qn_bool qn_pp_key_enable_fetching_from_callback_response(qn_json_object_ptr pp)
{
    return qn_json_set_integer(pp, "callbackFetchKey", 1);
} // qn_pp_key_enable_fetching_from_callback_response

qn_bool qn_pp_key_make_from_template(qn_json_object_ptr pp, const qn_string key_template)
{
    return qn_json_set_string(pp, "saveKey", key_template);
} // qn_pp_key_make_from_template

qn_bool qn_pp_auto_delete_after_days(qn_json_object_ptr pp, qn_uint32 days)
{
    return qn_json_set_integer(pp, "deleteAfterDays", days);
} // qn_pp_auto_delete_after_days

qn_string qn_pp_to_uptoken(qn_json_object_ptr pp, qn_mac_ptr mac)
{
    qn_string str = qn_json_object_to_string(pp);
    if (!str) return NULL;

    return qn_mac_make_uptoken(mac, qn_str_cstr(str), qn_str_size(str));
} // qn_pp_to_uptoken

// ---- Authorization Functions ----

qn_mac_ptr qn_mac_create(const char * access_key, const char * secret_key)
{
    qn_mac_ptr new_mac = malloc(sizeof(qn_mac_ptr));
    if (!new_mac) return NULL;

    new_mac->access_key = qn_str_duplicate(access_key);
    if (!new_mac->access_key) {
        free(new_mac);
        return NULL;
    } // if

    new_mac->secret_key = qn_str_duplicate(secret_key);
    if (!new_mac->secret_key) {
        free(new_mac->secret_key);
        free(new_mac);
        return NULL;
    } // if

    return new_mac;
} // qn_mac_create

void qn_mac_destroy(qn_mac_ptr mac)
{
    if (mac) {
        qn_str_destroy(mac->secret_key);
        qn_str_destroy(mac->access_key);
        free(mac);
    } // if
} // qn_mac_destroy

qn_string qn_mac_make_uptoken(qn_mac_ptr mac, const char * pp_str, qn_size pp_str_size)
{
    qn_string sign = NULL;
    qn_string encoded_pp = NULL;
    qn_string encoded_digest = NULL;
    char digest[EVP_MAX_MD_SIZE + 1];
    unsigned int digest_size = sizeof(digest);
    HMAC_CTX ctx;

    encoded_pp = qn_str_encode_base64_urlsafe(pp_str, pp_str_size);
    if (!encoded_pp) return NULL;

    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, qn_str_cstr(mac->secret_key), qn_str_size(mac->secret_key), EVP_sha1(), NULL);
    HMAC_Update(&ctx, (const unsigned char *)qn_str_cstr(encoded_pp), qn_str_size(encoded_pp));
    HMAC_Final(&ctx, (unsigned char *)digest, &digest_size);
    HMAC_CTX_cleanup(&ctx);

    encoded_digest = qn_str_encode_base64_urlsafe(digest, digest_size);
    sign = qn_str_join(":", mac->access_key, encoded_digest, encoded_pp);
    qn_str_destroy(encoded_digest);
    qn_str_destroy(encoded_pp);
    return sign;
} // qn_mac_make_uptoken

qn_string qn_mac_make_dnurl(qn_mac_ptr mac, const qn_string url, qn_uint32 deadline);

#ifdef __cplusplus
}
#endif

