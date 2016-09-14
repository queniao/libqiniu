#ifndef __QN_BASE_DS_ETABLE_H__
#define __QN_BASE_DS_ETABLE_H__

#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of entry table ----

struct _QN_ETABLE;
typedef struct _QN_ETABLE * qn_etable_ptr;

QN_API extern qn_etable_ptr qn_etbl_create(const char * restrict deli);
QN_API extern void qn_etbl_destroy(qn_etable_ptr etbl);
QN_API extern void qn_etbl_reset(qn_etable_ptr etbl);

QN_API extern const qn_string * qn_etbl_entries(qn_etable_ptr etbl);
QN_API extern int qn_etbl_size(qn_etable_ptr etbl);

QN_API extern qn_bool qn_etbl_set_raw(qn_etable_ptr etbl, const char * restrict key, size_t key_size, const char * restrict val, size_t val_size);

static inline qn_bool qn_etbl_set_text(qn_etable_ptr etbl, const char * restrict key, const char * restrict val, size_t val_size)
{
    return qn_etbl_set_raw(etbl, key, strlen(key), val, strlen(val));
}

static inline qn_bool qn_etbl_set_string(qn_etable_ptr etbl, const char * restrict key, const char * restrict val)
{
    return qn_etbl_set_raw(etbl, key, strlen(key), val, strlen(val));
}

QN_API extern qn_bool qn_etbl_set_integer(qn_etable_ptr etbl, const char * restrict key, int value);

#ifdef __cplusplus
}
#endif

#endif // __QN_BASE_DS_ETABLE_H__
