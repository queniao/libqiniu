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

QN_SDK extern qn_size qn_b64_encode_urlsafe(char * restrict encoded_str, qn_size encoded_cap, const char * restrict bin, qn_size bin_size, qn_uint32 opts);
QN_SDK extern qn_size qn_b64_decode_urlsafe(char * restrict decoded_bin, qn_size decoded_cap, const char * restrict str, qn_size str_size, qn_uint32 opts);

#ifdef __cplusplus
}
#endif

#endif // __QN_BASE64_H__
