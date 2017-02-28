#ifndef __QN_AUTH_H__
#define __QN_AUTH_H__

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"
#include "qiniu/base/json.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Authorization ----

struct _QN_MAC;
typedef struct _QN_MAC * qn_mac_ptr;

QN_SDK extern qn_mac_ptr qn_mac_create(const char * restrict access_key, const char * restrict secret_key);
QN_SDK extern void qn_mac_destroy(qn_mac_ptr restrict mac);

QN_SDK extern const qn_string qn_mac_make_uptoken(qn_mac_ptr restrict mac, const char * restrict pp, size_t pp_size);
QN_SDK extern const qn_string qn_mac_make_acctoken(qn_mac_ptr restrict mac, const char * restrict url, const char * restrict body, size_t body_size);
QN_SDK extern const qn_string qn_mac_make_dnurl(qn_mac_ptr restrict mac, const char * restrict url, qn_uint32 deadline);

#ifdef __cplusplus
}
#endif

#endif // __QN_AUTH_H__

