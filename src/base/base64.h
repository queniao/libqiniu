#ifndef __QN_BASE64_H__
#define __QN_BASE64_H__ 1

#include "base/basic_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define QN_B64_APPEND_PADDING 1

extern int qn_b64_encode_urlsafe(char * restrict encoded_str, qn_size encoded_size, const char * restrict bin, size_t bin_size, int opts);
extern int qn_b64_decode_urlsafe(char * restrict bin, qn_size bin_size, const char * restrict encoded_str, size_t encoded_size, int opts);

#ifdef __cplusplus
}
#endif

#endif // __QN_BASE64_H__
