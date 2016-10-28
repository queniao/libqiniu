#include "qiniu/base/errors.h"
#include "qiniu/misc.h"

#ifdef __cplusplus
extern "C"
{
#endif

QN_API qn_string qn_misc_encode_uri(const char * restrict bucket, const char * restrict key)
{
    char * buf;
    size_t buf_size;
    size_t bkt_size;
    size_t key_size;
    qn_string encoded_uri;

    buf_size = bkt_size = strlen(bucket);
    if (key) {
        key_size = strlen(key);
        buf_size += 1 + key_size;
    } // if

    buf = malloc(buf_size + 1);
    if (!buf) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    memcpy(buf, bucket, bkt_size);
    if (key) {
        buf[bkt_size] = ':';
        memcpy(buf + bkt_size + 1, key, key_size);
    } // if

    buf[buf_size] = '\0';

    encoded_uri = qn_cs_encode_base64_urlsafe(buf, buf_size);
    free(buf);
    return encoded_uri;
}

#ifdef __cplusplus
}
#endif

