#ifndef __QN_OS_TIME_H__
#define __QN_OS_TIME_H__

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Time Functions (abbreviation: tm) ----

QN_SDK extern qn_time qn_tm_time(void);
QN_SDK extern qn_string qn_tm_to_string(qn_time tm);
QN_SDK extern qn_ssize qn_tm_format_timestamp(qn_time tm, char * restrict buf, qn_size buf_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_OS_TIME_H__

