/***************************************************************************//**
* @file qiniu/cdn.h
* @brief This header file declares all core functions of Qiniu Cloud Fusion CDN.
*
* AUTHOR      : liangtao@qiniu.com (QQ: 510857)
* COPYRIGHT   : 2017(c) Shanghai Qiniu Information Technologies Co., Ltd.
* DESCRIPTION : TODO
*******************************************************************************/

#ifndef __QN_CDN_H__
#define __QN_CDN_H__

#include "qiniu/base/string.h"
#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

QN_API extern qn_string qn_cdn_make_dnurl_with_deadline(const char * restrict key, const char * restrict url, qn_uint32 deadline);

#ifdef __cplusplus
}
#endif

#endif // __QN_CDN_H__
