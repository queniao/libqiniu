#include <openssl/md5.h>
#include "qiniu/cdn.h"

#ifdef __cplusplus
extern "C"
{
#endif

static const char qn_cdn_hex_map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

typedef union _QN_CDN_UNIX_EPOCH
{
    qn_uint32 second;
    char bytes[4];
} qn_cdn_unix_epoch_un;

static qn_bool qn_cdn_percent_encode_check(int c)
{
    if (c == '/') return qn_false;
    return qn_cs_percent_encode_check(c);
}

QN_API qn_string qn_cdn_make_dnurl_with_deadline(const char * restrict key, const char * restrict url, qn_uint32 deadline)
{
    // ---- This function is based on the algorithm described on https://support.qiniu.com/question/195128 .
    int i;
    const char * begin;
    const char * path;
    size_t path_size;
    char * encoded_path;
    size_t encoded_path_size;
    size_t url_size;
    size_t base_url_size;
    const char * query;
    qn_string sign_str;
    qn_string authed_url;
    unsigned char sign[MD5_DIGEST_LENGTH];
    char encoded_sign[MD5_DIGEST_LENGTH * 2];
    qn_cdn_unix_epoch_un epoch;
    char encoded_unix_epoch[sizeof(epoch.bytes) * 2];
    MD5_CTX md5_ctx;

    epoch.second = deadline;
    encoded_unix_epoch[0] = qn_cdn_hex_map[(epoch.bytes[3] >> 4) & 0xF];
    encoded_unix_epoch[1] = qn_cdn_hex_map[epoch.bytes[3] & 0xF];
    encoded_unix_epoch[2] = qn_cdn_hex_map[(epoch.bytes[2] >> 4) & 0xF];
    encoded_unix_epoch[3] = qn_cdn_hex_map[epoch.bytes[2] & 0xF];
    encoded_unix_epoch[4] = qn_cdn_hex_map[(epoch.bytes[1] >> 4) & 0xF];
    encoded_unix_epoch[5] = qn_cdn_hex_map[epoch.bytes[1] & 0xF];
    encoded_unix_epoch[6] = qn_cdn_hex_map[(epoch.bytes[0] >> 4) & 0xF];
    encoded_unix_epoch[7] = qn_cdn_hex_map[epoch.bytes[0] & 0xF];

    url_size = posix_strlen(url);

    begin = posix_strstr(url, "://"); 
    if (! begin) return NULL;

    query = posix_strchr(begin + 3, '?');
    if (! query) query = url + url_size;

    path = posix_strchr(begin + 3, '/');
    if (! path) path = url + url_size;

    if (path < query) {
        // Case 1: http://xxx.com/path/to/file
        // Case 2: http://xxx.com/path/to/file/
        // Case 3: http://xxx.com/path//to//file
        // Case 4: http://xxx.com/path//to//file/
        // Case 5: http://xxx.com/path/to/file?imageView2/1/w/100/h/100
        // Case 6: http://xxx.com/path//to//file?imageView2/1/w/100/h/100
        // Case 7: http://xxx.com/path/to/file/?imageView2/1/w/100/h/100
        // Case 8: http://xxx.com/path//to//file/?imageView2/1/w/100/h/100

        base_url_size = path - url;
        path_size = query - path;
        encoded_path_size = qn_cs_percent_encode_in_buffer_with_checker(NULL, -1, path, path_size, &qn_cdn_percent_encode_check);
        if (encoded_path_size > path_size) {
            encoded_path = malloc(encoded_path_size + 1);
            if (! encoded_path) return NULL;

            qn_cs_percent_encode_in_buffer_with_checker(encoded_path, encoded_path_size, path, path_size, &qn_cdn_percent_encode_check);
            encoded_path[encoded_path_size] = '\0';
        } else if (encoded_path_size == path_size) {
            encoded_path = (char *)path;
        } else {
            // TODO: Set an appropriate error.
            return NULL;
        } // if
    } else {
        // Case 9: http://xxx.com
        // Case 10: http://xxx.com?imageView2/1/w/100/h/100
        base_url_size = query - url;

        path = "";
        path_size = 0;
        encoded_path = "";
        encoded_path_size = 0;
    } // if

    sign_str = qn_cs_sprintf("%s%.*s%.*s", key, encoded_path_size, encoded_path, sizeof(encoded_unix_epoch), encoded_unix_epoch);
    if (! sign_str) {
        if (encoded_path != path) free(encoded_path);
        return NULL;
    } // if

    MD5_Init(&md5_ctx);
    MD5_Update(&md5_ctx, qn_str_cstr(sign_str), qn_str_size(sign_str));
    MD5_Final((unsigned char *)sign, &md5_ctx);
    qn_str_destroy(sign_str);

    for (i = 0; i < MD5_DIGEST_LENGTH; i += 1) {
        encoded_sign[i * 2] = qn_cdn_hex_map[(sign[i] >> 4) & 0xF];
        encoded_sign[i * 2 + 1] = qn_cdn_hex_map[sign[i] & 0xF];
    } // if
    
    if (query != url + url_size) {
        authed_url = qn_cs_sprintf("%.*s%.*s%s&sign=%.*s&t=%.*s", base_url_size, url, encoded_path_size, encoded_path, query, sizeof(encoded_sign), encoded_sign, sizeof(encoded_unix_epoch), encoded_unix_epoch);
    } else {
        authed_url = qn_cs_sprintf("%.*s%.*s?sign=%.*s&t=%.*s", base_url_size, url, encoded_path_size, encoded_path, sizeof(encoded_sign), encoded_sign, sizeof(encoded_unix_epoch), encoded_unix_epoch);
    } // if
    if (encoded_path != path) free(encoded_path);

    return authed_url;
}

#ifdef __cplusplus
}
#endif
