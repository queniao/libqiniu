#include <openssl/hmac.h>
#include <openssl/err.h>

#include "qiniu/base/json_formatter.h"
#include "qiniu/auth.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of Authorization ----

typedef struct _QN_MAC
{
    qn_string access_key;
    qn_string secret_key;
} qn_mac;

QN_SDK qn_mac_ptr qn_mac_create(const char * restrict access_key, const char * restrict secret_key)
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

QN_SDK void qn_mac_destroy(qn_mac_ptr restrict mac)
{
    if (mac) {
        qn_str_destroy(mac->secret_key);
        qn_str_destroy(mac->access_key);
        free(mac);
    } // if
}

static int qn_mac_hmac_update(HMAC_CTX * restrict ctx, const unsigned char * restrict data, qn_size data_size)
{
#define QN_MAC_HMAC_MAX_WRITING_BYTES ( ((~((int)0)) << 1) >> 1 )
    int writing_bytes;
    qn_size rem_size = data_size;
    const unsigned char * pos = data;

    while (rem_size > 0) {
        writing_bytes = (rem_size > QN_MAC_HMAC_MAX_WRITING_BYTES) ? (QN_MAC_HMAC_MAX_WRITING_BYTES) : (rem_size & QN_MAC_HMAC_MAX_WRITING_BYTES);
        if (! HMAC_Update(ctx, (const unsigned char *)pos, writing_bytes)) {
            qn_err_3rdp_set_openssl_error_occurred(ERR_get_error());
            return 0;
        } // if
        pos += writing_bytes;
        rem_size -= writing_bytes;
    } // while
    return 1;
#undef QN_MAC_HMAC_MAX_WRITING_BYTES
}

QN_SDK const qn_string qn_mac_make_uptoken(qn_mac_ptr restrict mac, const char * restrict pp, qn_size pp_size)
{
    qn_string sign;
    qn_string encoded_pp;
    qn_string encoded_digest;
    char digest[EVP_MAX_MD_SIZE + 1];
    unsigned int digest_size = sizeof(digest);
    HMAC_CTX ctx;

    encoded_pp = qn_cs_encode_base64_urlsafe(pp, pp_size);
    if (!encoded_pp) return NULL;

    HMAC_CTX_init(&ctx);

    if (! HMAC_Init_ex(&ctx, qn_str_cstr(mac->secret_key), qn_str_size(mac->secret_key), EVP_sha1(), NULL)) {
        qn_err_3rdp_set_openssl_error_occurred(ERR_get_error());
        return NULL;
    } // if

    if (! qn_mac_hmac_update(&ctx, (const unsigned char *)qn_str_cstr(encoded_pp), qn_str_size(encoded_pp))) return NULL;

    if (! HMAC_Final(&ctx, (unsigned char *)digest, &digest_size)) {
        qn_err_3rdp_set_openssl_error_occurred(ERR_get_error());
        return NULL;
    } // if

    HMAC_CTX_cleanup(&ctx);

    encoded_digest = qn_cs_encode_base64_urlsafe(digest, digest_size);
    sign = qn_str_join_3(":", mac->access_key, encoded_digest, encoded_pp);
    qn_str_destroy(encoded_digest);
    qn_str_destroy(encoded_pp);
    return sign;
}

QN_SDK const qn_string qn_mac_make_acctoken(qn_mac_ptr restrict mac, const char * restrict url, const char * restrict body, qn_size body_size)
{
    qn_string encoded_digest;
    qn_string acctoken;
    const char * path;
    char digest[EVP_MAX_MD_SIZE + 1];
    unsigned int digest_size = sizeof(digest);
    HMAC_CTX ctx;

    path = qn_str_find_substring(url, "://");
    if (! path) {
        qn_err_set_invalid_argument();
        return NULL;
    } // if
    path += 3;

    path = qn_str_find_char(path, '/');
    if (! path) {
        qn_err_set_invalid_argument();
        return NULL;
    } // if

    HMAC_CTX_init(&ctx);

    if (! HMAC_Init_ex(&ctx, qn_str_cstr(mac->secret_key), qn_str_size(mac->secret_key), EVP_sha1(), NULL)) {
        qn_err_3rdp_set_openssl_error_occurred(ERR_get_error());
        return NULL;
    } // if

    qn_mac_hmac_update(&ctx, (const unsigned char *) path, posix_strlen(url) - (path - url));

    if (! HMAC_Update(&ctx, (const unsigned char *) "\n", 1)) {
        qn_err_3rdp_set_openssl_error_occurred(ERR_get_error());
        return NULL;
    } // if

    if (body && body_size > 0) {
        if (! qn_mac_hmac_update(&ctx, (const unsigned char *)body, body_size)) return NULL;
    } // if

    if (! HMAC_Final(&ctx, (unsigned char *)digest, &digest_size)) {
        qn_err_3rdp_set_openssl_error_occurred(ERR_get_error());
        return NULL;
    } // if

    HMAC_CTX_cleanup(&ctx);

    encoded_digest = qn_cs_encode_base64_urlsafe(digest, digest_size);
    if (!encoded_digest) return NULL;

    acctoken = qn_str_join_2(":", mac->access_key, encoded_digest);
    qn_str_destroy(encoded_digest);
    return acctoken;
}

QN_SDK const qn_string qn_mac_make_dnurl(qn_mac_ptr restrict mac, const char * restrict url, qn_uint32 deadline)
{
    qn_string deadline_str = NULL;
    qn_string url_with_deadline;
    qn_string url_with_token;
    qn_string encoded_digest;
    char * buf;
    const char * path;
    const char * end;
    int base_size;
    int encoded_size;
    char digest[EVP_MAX_MD_SIZE + 1];
    unsigned int digest_size = sizeof(digest);
    HMAC_CTX ctx;

    path = qn_str_find_substring(url, "://");
    if (! path) {
        qn_err_set_invalid_argument();
        return NULL;
    } // if
    path += 3;

    end = qn_str_find_char(path, '/');
    if (! end) {
        qn_err_set_invalid_argument();
        return NULL;
    } // if
    path = end + 1;
    base_size = path - url;

    end = qn_str_find_char(end, '?');
    if (! end) end = url + posix_strlen(url);

    deadline_str = qn_cs_sprintf("%u", deadline);
    if (! deadline_str) return NULL;

    encoded_size = qn_cs_percent_encode_in_buffer(NULL, 0, path, end - path);
    if (encoded_size > (end - path)) {
        // Case 1 : The path part contains non-URL-safe characters.
        buf = malloc(base_size + encoded_size + 1);
        if (! buf) {
            qn_str_destroy(deadline_str);
            qn_err_set_out_of_memory();
            return NULL;
        } // if
        buf[base_size + encoded_size + 1] = '\0';

        memcpy(buf, url, base_size); // Copy the schema and domain parts.
        qn_cs_percent_encode_in_buffer(buf + base_size, encoded_size, path, end - path); // Encode the path parts.
        if (*end == '?') {
            url_with_deadline = qn_cs_concat(buf, end, "&e=", qn_str_cstr(deadline_str), NULL);
        } else {
            url_with_deadline = qn_cs_concat(buf, "?e=", qn_str_cstr(deadline_str), NULL);
        } // if
        free(buf);
    } else {
        // Case 2 : The path part contains only URL-safe characters.
        if (*end == '?') {
            url_with_deadline = qn_cs_concat(url, "&e=", qn_str_cstr(deadline_str), NULL);
        } else {
            url_with_deadline = qn_cs_concat(url, "?e=", qn_str_cstr(deadline_str), NULL);
        } // if
    } // if
    qn_str_destroy(deadline_str);
    if (! url_with_deadline) return NULL;

    HMAC_CTX_init(&ctx);

    if (! HMAC_Init_ex(&ctx, qn_str_cstr(mac->secret_key), qn_str_size(mac->secret_key), EVP_sha1(), NULL)) {
        qn_err_3rdp_set_openssl_error_occurred(ERR_get_error());
        return NULL;
    } // if

    if (! qn_mac_hmac_update(&ctx, (const unsigned char *)qn_str_cstr(url_with_deadline), qn_str_size(url_with_deadline))) return NULL;

    if (! HMAC_Final(&ctx, (unsigned char *)digest, &digest_size)) {
        qn_err_3rdp_set_openssl_error_occurred(ERR_get_error());
        return NULL;
    } // if

    HMAC_CTX_cleanup(&ctx);

    encoded_digest = qn_cs_encode_base64_urlsafe(digest, digest_size);
    if (!encoded_digest) {
        qn_str_destroy(url_with_deadline);
        return NULL;
    } // if

    url_with_token = qn_cs_sprintf("%s&token=%s:%s", url_with_deadline, qn_str_cstr(mac->access_key), encoded_digest);
    qn_str_destroy(encoded_digest);
    qn_str_destroy(url_with_deadline);
    return url_with_token;
}

#ifdef __cplusplus
}
#endif

