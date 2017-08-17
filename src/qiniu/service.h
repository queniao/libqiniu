#ifndef __QN_SERVICE_H__
#define __QN_SERVICE_H__

#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Service Entry ----

typedef struct _QN_SVC_ENTRY
{
    qn_string base_url;
    qn_string hostname;
} qn_svc_entry_st, *qn_svc_entry_ptr;

static inline qn_bool qn_svc_is_https_entry(qn_svc_entry_ptr restrict ent)
{
    return posix_strstr(qn_str_cstr(ent->base_url), "https") == qn_str_cstr(ent->base_url);
}

// ---- Declaration of Service ----

typedef enum
{
    QN_SVC_UP = 0,
    QN_SVC_IO,
    QN_SVC_RS,
    QN_SVC_RSF,
    QN_SVC_API
} qn_svc_type;

struct _QN_SERVICE;
typedef struct _QN_SERVICE * qn_service_ptr;

QN_SDK extern qn_service_ptr qn_svc_create(qn_svc_type type);
QN_SDK extern void qn_svc_destroy(qn_service_ptr restrict svc);

QN_SDK extern unsigned int qn_svc_entry_count(qn_service_ptr restrict svc);
QN_SDK extern qn_svc_entry_ptr qn_svc_get_entry(qn_service_ptr restrict svc, unsigned int n);
QN_SDK extern qn_bool qn_svc_add_entry(qn_service_ptr restrict svc, qn_svc_entry_ptr restrict ent);

#ifdef __cplusplus
}
#endif

#endif // __QN_SERVICE_H__
