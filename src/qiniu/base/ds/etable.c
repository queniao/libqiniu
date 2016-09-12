#include <string.h>
#include <strings.h>

#include "qiniu/base/errors.h"
#include "qiniu/base/string.h"
#include "qiniu/base/ds/etable.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of entry table ----

typedef unsigned short qn_etbl_pos;

typedef struct _QN_ETABLE
{
    qn_string deli;
    qn_string * entries;
    qn_etbl_pos cnt;
    qn_etbl_pos cap;
} qn_etable;

QN_API qn_etable_ptr qn_etbl_create(const char * restrict deli)
{
    qn_etable_ptr new_etbl = calloc(1, sizeof(qn_etable));
    if (!new_etbl) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_etbl->deli = qn_cs_duplicate(deli);
    if (!new_etbl->deli) {
        free(new_etbl);
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_etbl->cnt = 0;
    new_etbl->cap = 4;

    new_etbl->entries = calloc(new_etbl->cap, sizeof(qn_string));
    if (!new_etbl->entries) {
        free(new_etbl);
        qn_err_set_no_enough_memory();
        return NULL;
    } // if
    return new_etbl;
}

QN_API void qn_etbl_destroy(qn_etable_ptr etbl)
{
    if (etbl) {
        qn_etbl_reset(etbl);
        qn_str_destroy(etbl->deli);
        free(etbl->entries);
        free(etbl);
    } // if
}

QN_API void qn_etbl_reset(qn_etable_ptr etbl)
{
    while (etbl->cnt > 0) qn_str_destroy(etbl->entries[--etbl->cnt]);
}

QN_API const qn_string * qn_etbl_entries(qn_etable_ptr etbl)
{
    return etbl->entries;
}

QN_API int qn_etbl_size(qn_etable_ptr etbl)
{
    return etbl->cnt;
}

static qn_etbl_pos qn_etbl_bsearch(qn_etable_ptr etbl, const char * key, int key_size, int * ord)
{
    qn_etbl_pos mid_key_size;
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

static qn_bool qn_etbl_augment(qn_etable_ptr etbl)
{
    qn_etbl_pos new_cap = etbl->cap + (etbl->cap >> 1); // 1.5 times
    qn_string * new_entries = calloc(new_cap, sizeof(qn_string));
    if (!new_entries) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_entries, etbl->entries, sizeof(qn_string) * etbl->cnt);
    free(etbl->entries);
    etbl->entries = new_entries;
    etbl->cap = new_cap;
    return qn_true;
}

static qn_bool qn_etbl_set_entry(qn_etable_ptr etbl, const char * restrict key, int key_size, const char * restrict val, int val_size)
{
    int ord;
    qn_string new_entry = NULL;
    qn_etbl_pos pos = 0;

    if (etbl->cnt == etbl->cap && !qn_etbl_augment(etbl)) return qn_false;
    if (key_size < 0) key_size = strlen(key);

    new_entry = qn_cs_sprintf("%.*s%s%.*s", key_size, key, etbl->deli, val_size, val);
    if (!new_entry) {
        qn_err_set_no_enough_memory();
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

QN_API qn_bool qn_etbl_set_string(qn_etable_ptr etbl, const char * restrict key, int key_size, const char * restrict val, int val_size)
{
    return qn_etbl_set_entry(etbl, key, key_size, val, val_size);
}

QN_API qn_bool qn_etbl_set_integer(qn_etable_ptr etbl, const char * restrict key, int key_size, int value)
{
    qn_bool ret;
    qn_string encoded_val = qn_cs_sprintf("%d", value);
    ret = qn_etbl_set_entry(etbl, key, key_size, qn_str_cstr(encoded_val), qn_str_size(encoded_val));
    qn_str_destroy(encoded_val);
    return ret;
}

#ifdef __cplusplus
}
#endif

