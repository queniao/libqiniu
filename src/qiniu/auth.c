#include <openssl/hmac.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/auth.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_MAC
{
    qn_string access_key;
    qn_string secret_key;
} qn_mac;

// ---- Definition of Put Policy ----

QN_API qn_json_object_ptr qn_pp_create(const char * restrict bucket, const char * restrict key, qn_uint32 deadline)
{
    qn_json_object_ptr pp = qn_json_create_object();
    if (pp) {
        if (!qn_pp_set_scope(pp, bucket, key)) return NULL;
        if (!qn_pp_set_deadline(pp, deadline)) return NULL;
    } // if
    return pp;
}

QN_API void qn_pp_destroy(qn_json_object_ptr restrict pp)
{
    qn_json_destroy_object(pp);
}

QN_API qn_bool qn_pp_set_scope(qn_json_object_ptr restrict pp, const char * restrict bucket, const char * restrict key)
{
    qn_bool ret;
    qn_string scope;
    if (key) {
        scope = qn_str_sprintf("%s:%s", bucket, key);
        if (!scope) return qn_false;
        ret = qn_json_set_string(pp, "scope", qn_str_cstr(scope));
        qn_str_destroy(scope);
    } else {
        ret = qn_json_set_string(pp, "scope", bucket);
    } // if
    return ret;
}

QN_API qn_bool qn_pp_set_deadline(qn_json_object_ptr restrict pp, qn_uint32 deadline)
{
    return qn_json_set_integer(pp, "deadline", deadline);
}

QN_API qn_bool qn_pp_dont_overwrite(qn_json_object_ptr restrict pp)
{
    return qn_json_set_integer(pp, "insertOnly", 1);
}

QN_API qn_bool qn_pp_return_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict body)
{
    if (!qn_json_set_string(pp, "returnUrl", url)) return qn_false;
    if (body) return qn_json_set_string(pp, "returnBody", body);
    return qn_true;
}

QN_API qn_bool qn_pp_return_to_client(qn_json_object_ptr restrict pp, const char * restrict body)
{
    return qn_json_set_string(pp, "returnBody", body);
}

QN_API qn_bool qn_pp_callback_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict host_name)
{
    if (!qn_json_set_string(pp, "callbackUrl", url)) return qn_false;
    if (host_name) return qn_json_set_string(pp, "callbackHost", host_name);
    return qn_true;
}

QN_API qn_bool qn_pp_callback_with_body(qn_json_object_ptr restrict pp, const char * restrict body, const char * restrict mime_type)
{
    if (!qn_json_set_string(pp, "callbackBody", body)) return qn_false;
    if (mime_type) return qn_json_set_string(pp, "callbackBodyType", mime_type);
    return qn_true;
}

QN_API qn_bool qn_pp_pfop_set_commands(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char * restrict cmd1, const char * restrict cmd2, ...)
{
    va_list ap;
    qn_bool ret;
    qn_string ops;

    if (!cmd2) {
        // Only one command passed.
        ret = qn_json_set_string(pp, "persistentOps", cmd1);
    } else {
        va_start(ap, cmd2);
        ops = qn_cs_join_va(";", cmd1, cmd2, ap);
        va_end(ap);
        if (!ops) return qn_false;
        ret = qn_json_set_string(pp, "persistentOps", ops);
        qn_str_destroy(ops);
    } // if

    if (ret && pipeline && strlen(pipeline) > 0) ret = qn_json_set_string(pp, "persistentPipeline", pipeline);
    return ret;
}

QN_API qn_bool qn_pp_pfop_set_command_list(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char ** restrict cmds, int cmd_count)
{
    qn_bool ret = qn_false;
    qn_string ops = NULL;

    if (cmd_count == 1) {
        // Only one command passed.
        ret = qn_json_set_string(pp, "persistentOps", cmds[0]);
    } else {
        ops = qn_cs_join_list(";", cmds, cmd_count);
        if (!ops) return qn_false;
        ret = qn_json_set_string(pp, "persistentOps", ops);
        qn_str_destroy(ops);
    } // if
    
    if (ret && pipeline && strlen(pipeline) > 0) ret = qn_json_set_string(pp, "persistentPipeline", pipeline);
    return ret;
}

QN_API qn_bool qn_pp_pfop_notify_to_server(qn_json_object_ptr restrict pp, const char * restrict url)
{
    return qn_json_set_string(pp, "persistentNotifyUrl", url);
}

QN_API qn_bool qn_pp_mime_enable_auto_detecting(qn_json_object_ptr restrict pp)
{
    return qn_json_set_integer(pp, "detectMime", 1);
}

QN_API qn_bool qn_pp_mime_allow(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...)
{
    va_list ap;
    qn_bool ret = qn_false;
    qn_string mime_str = NULL;

    if (!mime2) return qn_json_set_string(pp, "mimeLimit", mime1); // Only one mime passed.

    va_start(ap, mime2);
    mime_str = qn_cs_join_va(";", mime1, mime2, ap);
    va_end(ap);
    if (!mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", mime_str);
    qn_str_destroy(mime_str);
    return ret;
}

QN_API qn_bool qn_pp_mime_allow_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, int mime_count)
{
    qn_bool ret = qn_false;
    qn_string mime_str = NULL;
    
    if (mime_count == 1) return qn_json_set_string(pp, "mimeLimit", mime_list[0]);

    mime_str = qn_cs_join_list(";", mime_list, mime_count);
    if (!mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", mime_str);
    qn_str_destroy(mime_str);
    return ret;
}

QN_API qn_bool qn_pp_mime_deny(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...)
{
    va_list ap;
    qn_bool ret;
    qn_string deny_mime_str;
    qn_string mime_str;

    if (!mime2) {
        deny_mime_str = qn_str_sprintf("!%s", mime1); // Only one mime passed.
    } else {
        va_start(ap, mime2);
        mime_str = qn_cs_join_va(";", mime1, mime2, ap);
        va_end(ap);
        if (!mime_str) return qn_false;
        deny_mime_str = qn_str_sprintf("!%s", mime_str);
        qn_str_destroy(mime_str);
        if (!deny_mime_str) return qn_false;
    } // if

    ret = qn_json_set_string(pp, "mimeLimit", deny_mime_str);
    qn_str_destroy(deny_mime_str);
    return ret;
}

QN_API qn_bool qn_pp_mime_deny_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, int mime_count)
{
    qn_bool ret;
    qn_string deny_mime_str;
    qn_string mime_str;
    
    if (mime_count == 1) { 
        deny_mime_str = qn_str_sprintf("!%s", mime_list[0]); // Only one mime passed.
    } else {
        mime_str = qn_cs_join_list(";", mime_list, mime_count);
        if (!mime_str) return qn_false;
        deny_mime_str = qn_str_sprintf("!%s", mime_str);
        qn_str_destroy(mime_str);
    } // if
    if (!deny_mime_str) return qn_false;

    ret = qn_json_set_string(pp, "mimeLimit", deny_mime_str);
    qn_str_destroy(deny_mime_str);
    return ret;
}

QN_API qn_bool qn_pp_fsize_set_minimum(qn_json_object_ptr restrict pp, qn_uint32 min_size)
{
    return qn_json_set_integer(pp, "fsizeMin", min_size);
}

QN_API qn_bool qn_pp_fsize_set_maximum(qn_json_object_ptr restrict pp, qn_uint32 max_size)
{
    return qn_json_set_integer(pp, "fsizeLimit", max_size);
}

QN_API qn_bool qn_pp_key_enable_fetching_from_callback_response(qn_json_object_ptr restrict pp)
{
    return qn_json_set_integer(pp, "callbackFetchKey", 1);
}

QN_API qn_bool qn_pp_key_make_from_template(qn_json_object_ptr restrict pp, const char * restrict key_template)
{
    return qn_json_set_string(pp, "saveKey", key_template);
}

QN_API qn_bool qn_pp_auto_delete_after_days(qn_json_object_ptr restrict pp, qn_uint32 days)
{
    return qn_json_set_integer(pp, "deleteAfterDays", days);
}

QN_API qn_string qn_pp_to_uptoken(qn_json_object_ptr restrict pp, qn_mac_ptr restrict mac)
{
    qn_string str = qn_json_object_to_string(pp);
    if (!str) return NULL;
    return qn_mac_make_uptoken(mac, qn_str_cstr(str), qn_str_size(str));
}

// ---- Definition of Authorization ----

QN_API qn_mac_ptr qn_mac_create(const char * restrict access_key, const char * restrict secret_key)
{
    qn_mac_ptr new_mac = malloc(sizeof(qn_mac_ptr));
    if (!new_mac) return NULL;

    new_mac->access_key = qn_cs_duplicate(access_key);
    if (!new_mac->access_key) {
        free(new_mac);
        return NULL;
    } // if

    new_mac->secret_key = qn_cs_duplicate(secret_key);
    if (!new_mac->secret_key) {
        free(new_mac->secret_key);
        free(new_mac);
        return NULL;
    } // if
    return new_mac;
}

QN_API void qn_mac_destroy(qn_mac_ptr restrict mac)
{
    if (mac) {
        qn_str_destroy(mac->secret_key);
        qn_str_destroy(mac->access_key);
        free(mac);
    } // if
}

QN_API const qn_string qn_mac_make_uptoken(qn_mac_ptr restrict mac, const char * restrict pp, qn_size pp_size)
{
    qn_string sign;
    qn_string encoded_pp;
    qn_string encoded_digest;
    char digest[EVP_MAX_MD_SIZE + 1];
    unsigned int digest_size = sizeof(digest);
    HMAC_CTX ctx;

    encoded_pp = qn_str_encode_base64_urlsafe(pp, pp_size);
    if (!encoded_pp) return NULL;

    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, qn_str_cstr(mac->secret_key), qn_str_size(mac->secret_key), EVP_sha1(), NULL);
    HMAC_Update(&ctx, (const unsigned char *)qn_str_cstr(encoded_pp), qn_str_size(encoded_pp));
    HMAC_Final(&ctx, (unsigned char *)digest, &digest_size);
    HMAC_CTX_cleanup(&ctx);

    encoded_digest = qn_str_encode_base64_urlsafe(digest, digest_size);
    sign = qn_str_join_3(":", mac->access_key, encoded_digest, encoded_pp);
    qn_str_destroy(encoded_digest);
    qn_str_destroy(encoded_pp);
    return sign;
}

QN_API const qn_string qn_mac_make_acctoken(qn_mac_ptr restrict mac, const char * restrict url, const char * restrict body, qn_size body_size)
{
    qn_string encoded_digest;
    qn_string acctoken;
    const char * begin;
    char digest[EVP_MAX_MD_SIZE + 1];
    unsigned int digest_size = sizeof(digest);
    HMAC_CTX ctx;

    begin = qn_str_find_substring(url, "://");
    if (!begin) {
        qn_err_set_invalid_argument();
        return NULL;
    } // if
    begin += 3;

    begin = qn_str_find_char(begin, '/');
    if (!begin) {
        qn_err_set_invalid_argument();
        return NULL;
    } // if

    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, qn_str_cstr(mac->secret_key), qn_str_size(mac->secret_key), EVP_sha1(), NULL);
    HMAC_Update(&ctx, (const unsigned char *)begin, strlen(url) - (begin - url));
    HMAC_Update(&ctx, (const unsigned char *)"\n", 1);

    if (body && body_size > 0) {
        HMAC_Update(&ctx, (const unsigned char *)body, body_size);
    } // if

    HMAC_Final(&ctx, (unsigned char *)digest, &digest_size);
    HMAC_CTX_cleanup(&ctx);

    encoded_digest = qn_str_encode_base64_urlsafe(digest, digest_size);
    if (!encoded_digest) return NULL;

    acctoken = qn_str_join_2(":", mac->access_key, encoded_digest);
    qn_str_destroy(encoded_digest);
    return acctoken;
}

QN_API const qn_string qn_mac_make_dnurl(qn_mac_ptr restrict mac, const char * restrict url, qn_uint32 deadline)
{
    qn_string url_with_deadline;
    qn_string url_with_token;
    qn_string encoded_digest;
    char * buf;
    const char * begin;
    const char * end;
    int url_size;
    int buf_size;
    char digest[EVP_MAX_MD_SIZE + 1];
    unsigned int digest_size = sizeof(digest);
    HMAC_CTX ctx;

    begin = qn_str_find_substring(url, "://");
    if (!begin) {
        qn_err_set_invalid_argument();
        return NULL;
    } // if
    begin += 3;

    end = qn_str_find_char(begin, '/');
    if (!end) {
        qn_err_set_invalid_argument();
        return NULL;
    } // if
    begin = end + 1;

    end = qn_str_find_char(end, '?');
    url_size = strlen(url);
    if (!end) end = url + url_size;

    buf_size = qn_str_percent_encode_in_buffer(NULL, 0, begin, end - begin);
    if (buf_size > (end - begin)) {
        // Need to percent encode the path of the url.
        buf = malloc(buf_size + 1);
        if (!buf) {
            qn_err_set_no_enough_memory();
            return NULL;
        } // if
        buf[buf_size + 1] = '\0';

        qn_str_percent_encode_in_buffer(buf, buf_size, begin, end - begin);
        if (*end == '?') {
            url_with_deadline = qn_str_sprintf("%.*s%.*s%.*s&e=%d", begin - url, url, buf_size, buf, url_size - (end - url), end, deadline);
        } else {
            url_with_deadline = qn_str_sprintf("%.*s%.*s?e=%d", begin - url, url, buf_size, buf, deadline);
        } // if
        free(buf);
    } else {
        if (*end == '?') {
            url_with_deadline = qn_str_sprintf("%.*s&e=%d", url_size, url, deadline);
        } else {
            url_with_deadline = qn_str_sprintf("%.*s?e=%d", url_size, url, deadline);
        }
    } // if
    if (!url_with_deadline) return NULL;

    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, qn_str_cstr(mac->secret_key), qn_str_size(mac->secret_key), EVP_sha1(), NULL);
    HMAC_Update(&ctx, (const unsigned char *)qn_str_cstr(url_with_deadline), qn_str_size(url_with_deadline));
    HMAC_Final(&ctx, (unsigned char *)digest, &digest_size);
    HMAC_CTX_cleanup(&ctx);

    encoded_digest = qn_str_encode_base64_urlsafe(digest, digest_size);
    if (!encoded_digest) {
        qn_str_destroy(url_with_deadline);
        return NULL;
    } // if

    url_with_token = qn_str_sprintf("%s&token=%s:%s", url_with_deadline, qn_str_cstr(mac->access_key), encoded_digest);
    qn_str_destroy(encoded_digest);
    qn_str_destroy(url_with_deadline);
    return url_with_token;
}

#ifdef __cplusplus
}
#endif

