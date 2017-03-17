#ifndef __QN_BASE_DS_ETABLE_H__
#define __QN_BASE_DS_ETABLE_H__

#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Entry Table ----

struct _QN_ETABLE;
typedef struct _QN_ETABLE * qn_etable_ptr;

QN_SDK extern qn_etable_ptr qn_etbl_create(const char * restrict deli);
QN_SDK extern void qn_etbl_destroy(qn_etable_ptr restrict etbl);
QN_SDK extern void qn_etbl_reset(qn_etable_ptr restrict etbl);

QN_SDK extern const qn_string * qn_etbl_entries(qn_etable_ptr restrict etbl);
QN_SDK extern int qn_etbl_count(qn_etable_ptr restrict etbl);

QN_SDK extern qn_string qn_etbl_get_entry(qn_etable_ptr restrict etbl, const char * restrict key);
QN_SDK extern const char * qn_etbl_get_value(qn_etable_ptr restrict etbl, const char * restrict key);
QN_SDK extern void qn_etbl_get_pair_raw(qn_etable_ptr restrict etbl, const qn_string ent, const char ** restrict key, size_t * restrict key_size, const char ** restrict val, size_t * restrict val_size);

QN_SDK extern qn_bool qn_etbl_set_raw(qn_etable_ptr restrict etbl, const char * restrict key, size_t key_size, const char * restrict val, size_t val_size);

static inline qn_bool qn_etbl_set_text(qn_etable_ptr restrict etbl, const char * restrict key, const char * restrict val, size_t val_size)
{
    return qn_etbl_set_raw(etbl, key, strlen(key), val, strlen(val));
}

static inline qn_bool qn_etbl_set_string(qn_etable_ptr restrict etbl, const char * restrict key, const char * restrict val)
{
    return qn_etbl_set_raw(etbl, key, strlen(key), val, strlen(val));
}

QN_SDK extern qn_bool qn_etbl_set_integer(qn_etable_ptr restrict etbl, const char * restrict key, int value);

QN_SDK extern void qn_etbl_unset(qn_etable_ptr restrict etbl, const char * restrict key);

// ---- Declaration of Entry Table Iterator ----

struct _QN_ETABLE_ITERATOR;
typedef struct _QN_ETABLE_ITERATOR * qn_etbl_iterator_ptr;

QN_SDK extern qn_etbl_iterator_ptr qn_etbl_itr_create(qn_etable_ptr restrict etbl);
QN_SDK extern void qn_etbl_itr_destroy(qn_etbl_iterator_ptr restrict itr);
QN_SDK extern void qn_etbl_itr_rewind(qn_etbl_iterator_ptr restrict itr);

QN_SDK extern qn_string qn_etbl_itr_next_entry(qn_etbl_iterator_ptr restrict itr);
QN_SDK extern qn_bool qn_etbl_itr_next_pair_raw(qn_etbl_iterator_ptr restrict itr, const char ** restrict key, size_t * restrict key_size, const char ** restrict val, size_t * restrict val_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_BASE_DS_ETABLE_H__
