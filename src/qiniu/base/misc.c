#include <time.h>

#include "qiniu/base/misc.h"

#ifdef __cplusplus
extern "C"
{
#endif

QN_API qn_uint32 qn_misc_localtime(void)
{
    return time(NULL);
}

#ifdef __cplusplus
}
#endif
