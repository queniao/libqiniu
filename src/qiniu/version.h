#ifndef __QN_VERSION_H__
#define __QN_VERSION_H__ 1

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_VERSION
{
    int major;
    int minor;
    int patch;
    const char * release;
} qn_version, *qn_version_ptr;

QN_SDK extern const char * qn_ver_get_full_string(void);
QN_SDK extern void qn_ver_get_numbers(qn_version_ptr restrict ver);

#ifdef __cplusplus
}
#endif

#endif // __QN_VERSION_H__

