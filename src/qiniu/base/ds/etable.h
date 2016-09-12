#ifndef __QN_ETABLE_H__
#define __QN_ETABLE_H__

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

QN_API extern qn_bool qn_etbl_set_string(qn_etable_ptr etbl, const char * restrict key, int key_size, const char * restrict val, int val_size);
QN_API extern qn_bool qn_etbl_set_integer(qn_etable_ptr etbl, const char * restrict key, int key_size, int value);

QN_API extern const qn_string * qn_etbl_entries(qn_etable_ptr etbl);
QN_API extern int qn_etbl_size(qn_etable_ptr etbl);

#ifdef __cplusplus
}
#endif

#endif // __QN_ETABLE_H__
