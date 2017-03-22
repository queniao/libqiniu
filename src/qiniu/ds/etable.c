#include <string.h>
#include <strings.h>

#include "qiniu/base/errors.h"
#include "qiniu/base/string.h"
#include "qiniu/ds/etable.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of Entry Table ----

typedef int qn_etbl_pos;

typedef struct _QN_ETABLE
{
    qn_string deli;
    qn_string * entries;
    qn_etbl_pos cnt;
    qn_etbl_pos cap;
} qn_etable;

QN_SDK qn_etable_ptr qn_etbl_create(const char * restrict deli)
{
    qn_etable_ptr new_etbl = calloc(1, sizeof(qn_etable));
    if (!new_etbl) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_etbl->deli = qn_cs_duplicate(deli);
    if (!new_etbl->deli) {
        free(new_etbl);
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_etbl->cnt = 0;
    new_etbl->cap = 4;

    new_etbl->entries = calloc(new_etbl->cap, sizeof(qn_string));
    if (!new_etbl->entries) {
        free(new_etbl);
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_etbl;
}

QN_SDK void qn_etbl_destroy(qn_etable_ptr restrict etbl)
{
    if (etbl) {
        qn_etbl_reset(etbl);
        qn_str_destroy(etbl->deli);
        free(etbl->entries);
        free(etbl);
    } // if
}

QN_SDK void qn_etbl_reset(qn_etable_ptr restrict etbl)
{
    while (etbl->cnt > 0) qn_str_destroy(etbl->entries[--etbl->cnt]);
}

QN_SDK const qn_string * qn_etbl_entries(qn_etable_ptr restrict etbl)
{
    return etbl->entries;
}

QN_SDK int qn_etbl_count(qn_etable_ptr restrict etbl)
{
    return etbl->cnt;
}

static qn_etbl_pos qn_etbl_bsearch(qn_etable_ptr restrict etbl, const char * key, qn_size key_size, int * ord)
{
    qn_size mid_key_size;
    qn_etbl_pos begin = 0;
    qn_etbl_pos end = etbl->cnt;
    qn_etbl_pos mid = 0;

    while (begin < end) {
        mid = begin + ((end - begin) / 2);
        mid_key_size = strstr(etbl->entries[mid], etbl->deli) - etbl->entries[mid];
        *ord = strncasecmp(etbl->entries[mid], key, (mid_key_size < key_size ? mid_key_size : key_size));
        if (*ord == 0) {
            if (mid_key_size < key_size) {
                *ord = -1;
            } else if (mid_key_size > key_size) {
                *ord = 1;
            } // if
        } // if
        if (*ord < 0) {
            begin = mid + 1;
        } else {
            end = mid;
        } // if
    } // while
    return begin;
}

static qn_bool qn_etbl_augment(qn_etable_ptr restrict etbl)
{
    qn_etbl_pos new_cap = etbl->cap + (etbl->cap >> 1); // 1.5 times
    qn_string * new_entries = calloc(new_cap, sizeof(qn_string));
    if (!new_entries) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_entries, etbl->entries, sizeof(qn_string) * etbl->cnt);
    free(etbl->entries);
    etbl->entries = new_entries;
    etbl->cap = new_cap;
    return qn_true;
}

QN_SDK qn_string qn_etbl_get_entry(qn_etable_ptr restrict etbl, const char * restrict key)
{
    int ord;
    qn_etbl_pos pos = qn_etbl_bsearch(etbl, key, strlen(key), &ord);
    return (pos == etbl->cnt) ? NULL : etbl->entries[pos];
}

QN_SDK const char * qn_etbl_get_value(qn_etable_ptr restrict etbl, const char * restrict key)
{
    int ord;
    qn_etbl_pos pos = qn_etbl_bsearch(etbl, key, strlen(key), &ord);
    return (pos == etbl->cnt) ? NULL : (strstr(etbl->entries[pos], etbl->deli) + qn_str_size(etbl->deli));
}

QN_SDK void qn_etbl_get_pair_raw(qn_etable_ptr restrict etbl, const qn_string ent, const char ** restrict key, qn_size * restrict key_size, const char ** restrict val, qn_size * restrict val_size)
{
    *key = ent;
    *key_size = strstr(ent, etbl->deli) - ent;
    *val = *key + *key_size + qn_str_size(etbl->deli);
    *val_size = (ent + qn_str_size(ent)) - *val;
}

static qn_bool qn_etbl_set_entry(qn_etable_ptr restrict etbl, const char * restrict key, qn_size key_size, const char * restrict val, qn_size val_size)
{
    int ord;
    qn_string new_entry;
    qn_etbl_pos pos;

    if (etbl->cnt == etbl->cap && !qn_etbl_augment(etbl)) return qn_false;

    new_entry = qn_cs_sprintf("%.*s%s%.*s", key_size, key, etbl->deli, val_size, val);
    if (!new_entry) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    pos = qn_etbl_bsearch(etbl, key, key_size, &ord);
    if (pos < etbl->cnt && ord == 0) {
        qn_str_destroy(etbl->entries[pos]);
        etbl->entries[pos] = new_entry;
    } else {
        memmove(&etbl->entries[pos+1], &etbl->entries[pos], sizeof(qn_string) * (etbl->cnt - pos));
        etbl->entries[pos] = new_entry;
        etbl->cnt += 1;
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_etbl_set_raw(qn_etable_ptr restrict etbl, const char * restrict key, qn_size key_size, const char * restrict val, qn_size val_size)
{
    return qn_etbl_set_entry(etbl, key, key_size, val, val_size);
}

QN_SDK qn_bool qn_etbl_set_integer(qn_etable_ptr restrict etbl, const char * restrict key, int value)
{
    qn_bool ret;
    qn_string encoded_val = qn_cs_sprintf("%d", value);
    ret = qn_etbl_set_entry(etbl, key, strlen(key), qn_str_cstr(encoded_val), qn_str_size(encoded_val));
    qn_str_destroy(encoded_val);
    return ret;
}

QN_SDK void qn_etbl_unset(qn_etable_ptr restrict etbl, const char * restrict key)
{
    int ord;
    qn_etbl_pos pos = qn_etbl_bsearch(etbl, key, strlen(key), &ord);

    if (pos == etbl->cnt) return;

    qn_str_destroy(etbl->entries[pos]);
    if (pos < etbl->cnt - 1) memmove(etbl->entries + pos, etbl->entries + pos + 1, sizeof(qn_string) * (etbl->cnt - pos - 1));
    etbl->cnt -= 1;
}

// ---- Definition of Entry Table Iterator ----

typedef struct _QN_ETABLE_ITERATOR
{
    qn_etable_ptr etbl;
    qn_etbl_pos pos;
} qn_etbl_iterator;

QN_SDK qn_etbl_iterator_ptr qn_etbl_itr_create(qn_etable_ptr restrict etbl)
{
    qn_etbl_iterator_ptr new_itr = calloc(1, sizeof(qn_etbl_iterator));
    if (!new_itr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_itr->etbl = etbl;
    new_itr->pos = 0;
    return new_itr;
}

QN_SDK void qn_etbl_itr_destroy(qn_etbl_iterator_ptr restrict itr)
{
    if (itr) {
        free(itr);
    } // if
}

QN_SDK void qn_etbl_itr_rewind(qn_etbl_iterator_ptr restrict itr)
{
    itr->pos = 0;
}

QN_SDK qn_string qn_etbl_itr_next_entry(qn_etbl_iterator_ptr restrict itr)
{
    if (itr->pos == itr->etbl->cnt) return NULL;
    return itr->etbl->entries[itr->pos++];
}

QN_SDK qn_bool qn_etbl_itr_next_pair_raw(qn_etbl_iterator_ptr restrict itr, const char ** restrict key, qn_size * restrict key_size, const char ** restrict val, qn_size * restrict val_size)
{
    qn_string ent;

    if (itr->pos == itr->etbl->cnt) return qn_false;
    ent = itr->etbl->entries[itr->pos++];

    qn_etbl_get_pair_raw(itr->etbl, ent, key, key_size, val, val_size);
    return qn_true;
}

#ifdef __cplusplus
}
#endif

