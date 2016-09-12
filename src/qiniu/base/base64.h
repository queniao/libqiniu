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

QN_API extern qn_size qn_b64_encode_urlsafe(char * restrict encoded_str, qn_size encoded_size, const char * restrict bin, size_t bin_size, int opts);
QN_API extern qn_size qn_b64_decode_urlsafe(char * restrict decoded_bin, qn_size decoded_size, const char * restrict str, size_t str_size, int opts);

#ifdef __cplusplus
}
#endif

#endif // __QN_BASE64_H__
