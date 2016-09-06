#ifndef __QN_ETABLE_H__
#define __QN_ETABLE_H__

#include "qiniu/base/string.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of entry table ----

struct _QN_ETABLE;
typedef struct _QN_ETABLE * qn_etable_ptr;

extern qn_etable_ptr qn_etbl_create(const char * restrict deli);
extern void qn_etbl_destroy(qn_etable_ptr etbl);
extern void qn_etbl_reset(qn_etable_ptr etbl);

extern qn_bool qn_etbl_set_string(qn_etable_ptr etbl, const char * restrict key, int key_size, const char * restrict val, int val_size);
extern qn_bool qn_etbl_set_integer(qn_etable_ptr etbl, const char * restrict key, int key_size, int value);

extern const qn_string * qn_etbl_entries(qn_etable_ptr etbl);
extern int qn_etbl_size(qn_etable_ptr etbl);

#ifdef __cplusplus
}
#endif

#endif // __QN_ETABLE_H__
