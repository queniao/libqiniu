#ifndef __QN_BASE64_H__
#define __QN_BASE64_H__ 1

#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum {
    QN_B64_APPEND_PADDING = 0x1
};

QN_SDK extern size_t qn_b64_encode_urlsafe(char * restrict encoded_str, size_t encoded_cap, const char * restrict bin, size_t bin_size, qn_uint32 opts);
QN_SDK extern size_t qn_b64_decode_urlsafe(char * restrict decoded_bin, size_t decoded_cap, const char * restrict str, size_t str_size, qn_uint32 opts);

#ifdef __cplusplus
}
#endif

#endif // __QN_BASE64_H__
