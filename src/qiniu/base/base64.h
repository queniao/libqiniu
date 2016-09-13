#ifndef __QN_BASE64_H__
#define __QN_BASE64_H__ 1

#include "qiniu/base/basic_types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum {
    QN_B64_APPEND_PADDING = 1
};

QN_API extern int qn_b64_encode_urlsafe(char * restrict encoded_str, int encoded_cap, const char * restrict bin, int bin_size, int opts);
QN_API extern int qn_b64_decode_urlsafe(char * restrict decoded_bin, int decoded_cap, const char * restrict str, int str_size, int opts);

#ifdef __cplusplus
}
#endif

#endif // __QN_BASE64_H__
