#include "qiniu/base/errors.h"
#include "qiniu/misc.h"

#ifdef __cplusplus
extern "C"
{
#endif

qn_string qn_misc_encode_uri(const char * restrict bucket, const char * restrict key)
{
    char * buf = NULL;
    int buf_size = 0;
    int bkt_size = 0;
    int key_size = 0;
    qn_string encoded_uri = NULL;

    buf_size = bkt_size = strlen(bucket);
    if (key) {
        key_size = strlen(key);
        buf_size += 1 + key_size;
    } // if

    buf = malloc(buf_size + 1);
    if (!buf) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    memcpy(buf, bucket, bkt_size);
    if (key) {
        buf[bkt_size] = ':';
        memcpy(buf + bkt_size + 1, key, key_size);
    } // if

    buf[buf_size] = '\0';

    encoded_uri = qn_str_encode_base64_urlsafe(buf, buf_size);
    free(buf);
    return encoded_uri;
}

#ifdef __cplusplus
}
#endif

