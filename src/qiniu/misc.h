#ifndef __QN_MISC_H__
#define __QN_MISC_H__

#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

QN_SDK extern qn_string qn_misc_encode_uri(const char * restrict bucket, const char * restrict key);

#ifdef __cplusplus
}
#endif

#endif // __QN_MISC_H__

