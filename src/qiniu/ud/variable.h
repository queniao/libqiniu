#ifndef __QN_UD_VARIABLE_H_
#define __QN_UD_VARIABLE_H_

#include "qiniu/base/ds/etable.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- User-Defined Variable Functions (abbreviation: ud_var) ----

struct _QN_UD_VARIABLE;
typedef struct _QN_UD_VARIABLE * qn_ud_variable_ptr;

static inline qn_ud_variable_ptr qn_ud_var_create(void)
{
    return (qn_ud_variable_ptr) qn_etbl_create("/");
}

static inline void qn_ud_var_destroy(qn_ud_variable_ptr restrict udv)
{
    qn_etbl_destroy((qn_etable_ptr) udv);
}

static inline void qn_ud_var_reset(qn_ud_variable_ptr restrict udv)
{
    qn_etbl_reset((qn_etable_ptr) udv);
}

static inline int qn_ud_var_count(qn_ud_variable_ptr restrict udv)
{
    return qn_etbl_count((qn_etable_ptr) udv);
}

QN_SDK extern qn_bool qn_ud_var_set_raw(qn_ud_variable_ptr restrict udv, const char * restrict key, qn_size key_size, const char * restrict val, qn_size val_size);

static inline qn_bool qn_ud_var_set_text(qn_ud_variable_ptr restrict udv, const char * restrict key, const char * restrict val, qn_size val_size)
{
    return qn_ud_var_set_raw(udv, key, posix_strlen(key), val, val_size);
}

static inline qn_bool qn_ud_var_set_string(qn_ud_variable_ptr restrict udv, const char * restrict key, const char * restrict val)
{
    return qn_ud_var_set_raw(udv, key, posix_strlen(key), val, posix_strlen(val));
}

static inline void qn_ud_var_unset(qn_ud_variable_ptr restrict udv, const char * restrict key)
{
    qn_etbl_unset((qn_etable_ptr) udv, key);
}

// ---- User-Defined Variable Iterator Functions (abbreviation: ud_var_itr) ----

struct _QN_UD_VAR_ITERATOR;
typedef struct _QN_UD_VAR_ITERATOR * qn_ud_var_iterator_ptr;

static inline qn_ud_var_iterator_ptr qn_ud_var_itr_create(qn_ud_variable_ptr restrict udv)
{
    return (qn_ud_var_iterator_ptr) qn_etbl_itr_create((qn_etable_ptr) udv);
}

static inline void qn_ud_var_itr_destroy(qn_ud_var_iterator_ptr restrict itr)
{
    qn_etbl_itr_destroy((qn_etbl_iterator_ptr) itr);
}

static inline void qn_ud_var_itr_rewind(qn_ud_var_iterator_ptr restrict itr)
{
    qn_etbl_itr_rewind((qn_etbl_iterator_ptr) itr);
}

static inline qn_string qn_ud_var_itr_next_entry(qn_ud_var_iterator_ptr restrict itr)
{
    return qn_etbl_itr_next_entry((qn_etbl_iterator_ptr) itr);
}

static inline qn_bool qn_ud_var_itr_next_pair_raw(qn_ud_var_iterator_ptr restrict itr, const char ** restrict key, qn_size * restrict key_size, const char ** restrict val, qn_size * restrict val_size)
{
    return qn_etbl_itr_next_pair_raw((qn_etbl_iterator_ptr) itr, key, key_size, val, val_size);
}

#ifdef __cplusplus
}
#endif

#endif // __QN_UD_VARIABLE_H_

