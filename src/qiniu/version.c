#include <stdio.h>
#include <string.h>

#include "qiniu/version.h"

#ifdef __cplusplus
extern "C"
{
#endif

static const char * qn_ver_full_string = "libqiniu-0.4.1";

QN_API const char * qn_ver_get_full_string(void)
{
    return qn_ver_full_string;
}

QN_API void qn_ver_get_numbers(qn_version_ptr restrict ver)
{
    memset(ver, 0, sizeof(qn_version));
    sscanf(qn_ver_full_string, "libqiniu-%d.%d.%d", &ver->major, &ver->minor, &ver->patch);
}

#ifdef __cplusplus
}
#endif
