#include <strings.h>

#include "qiniu/base/errors.h"
#include "qiniu/http_header.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of HTTP header ----

typedef unsigned short qn_http_hdr_pos;

typedef struct _QN_HTTP_HEADER
{
    qn_string * entries;
    qn_http_hdr_pos cnt;
    qn_http_hdr_pos cap;
} qn_http_header;

qn_http_header_ptr qn_http_hdr_create(void)
{
    qn_http_header_ptr new_hdr = calloc(1, sizeof(qn_http_header));
    if (!new_hdr) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_hdr->entries = calloc(8, sizeof(qn_string));
    if (!new_hdr->entries) {
        free(new_hdr);
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_hdr->cnt = 0;
    new_hdr->cap = 8;
    return new_hdr;
}

void qn_http_hdr_destroy(qn_http_header_ptr hdr)
{
    if (hdr) {
        qn_http_hdr_reset(hdr);
        free(hdr->entries);
        free(hdr);
    } // if
}

void qn_http_hdr_reset(qn_http_header_ptr hdr)
{
    while (hdr->cnt > 0) qn_str_destroy(hdr->entries[--hdr->cnt]);
}

qn_size qn_http_hdr_size(qn_http_header_ptr hdr)
{
    return hdr->cnt;
}

static qn_http_hdr_pos qn_http_hdr_bsearch(qn_http_header_ptr hdr, const char * key, qn_size key_size, int * ord)
{
    qn_http_hdr_pos mid_key_size;
    qn_http_hdr_pos begin = 0;
    qn_http_hdr_pos end = hdr->cnt;
    qn_http_hdr_pos mid = 0;

    while (begin < end) {
        mid = begin + ((end - begin) / 2);
        mid_key_size = strchr(hdr->entries[mid], ':') - hdr->entries[mid];
        *ord = strncasecmp(hdr->entries[mid], key, (mid_key_size < key_size ? mid_key_size : key_size));
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

qn_string qn_http_hdr_get_entry_raw(qn_http_header_ptr hdr, const char * key, qn_size key_size)
{
    int ord;
    qn_http_hdr_pos pos = qn_http_hdr_bsearch(hdr, key, key_size, &ord);
    if (pos == hdr->cnt) return NULL;
    return hdr->entries[pos];
}

qn_bool qn_http_hdr_get_raw(qn_http_header_ptr hdr, const char * key, qn_size key_size, const char ** val, qn_size * val_size)
{
    int ord;
    qn_http_hdr_pos pos = qn_http_hdr_bsearch(hdr, key, key_size, &ord);
    if (pos == hdr->cnt) return qn_false;
    *val = strchr(qn_str_cstr(hdr->entries[pos]), ':') + 2;
    *val_size = qn_str_size(hdr->entries[pos]) - (*val - qn_str_cstr(hdr->entries[pos]));
    return qn_true;
}

static qn_bool qn_http_hdr_augment(qn_http_header_ptr hdr)
{
    qn_http_hdr_pos new_cap = hdr->cap + (hdr->cap >> 1); // 1.5 times
    qn_string * new_entries = calloc(new_cap, sizeof(qn_string));
    if (!new_entries) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_entries, hdr->entries, sizeof(qn_string) * hdr->cnt);
    free(hdr->entries);
    hdr->entries = new_entries;
    hdr->cap = new_cap;
    return qn_true;
}

qn_bool qn_http_hdr_set_raw(qn_http_header_ptr hdr, const char * key, qn_size key_size, const char * val, qn_size val_size)
{
    int ord;
    qn_string new_val = NULL;
    qn_http_hdr_pos pos = 0;

    if (hdr->cnt == hdr->cap && !qn_http_hdr_augment(hdr)) return qn_false;

    new_val = qn_str_sprintf("%.*s: %.*s", key_size, key, val_size, val);
    if (!new_val) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    pos = qn_http_hdr_bsearch(hdr, key, key_size, &ord);
    if (pos < hdr->cnt && ord == 0) {
        qn_str_destroy(hdr->entries[pos]);
        hdr->entries[pos] = new_val;
    } else {
        memmove(&hdr->entries[pos+1], &hdr->entries[pos], sizeof(qn_string) * (hdr->cnt - pos));
        hdr->entries[pos] = new_val;
        hdr->cnt += 1;
    } // if
    return qn_true;
}

void qn_http_hdr_unset_raw(qn_http_header_ptr hdr, const char * key, qn_size key_size)
{
    int ord;
    qn_http_hdr_pos pos = qn_http_hdr_bsearch(hdr, key, key_size, &ord);
    if (pos < hdr->cnt) {
        qn_str_destroy(hdr->entries[pos]);
        hdr->cnt -= 1;
    } // if
}

// ---- Definition of HTTP header iterator ----

typedef struct _QN_HTTP_HDR_ITERATOR
{
    qn_http_header_ptr hdr;
    qn_http_hdr_pos pos;
} qn_http_hdr_iterator;

qn_http_hdr_iterator_ptr qn_http_hdr_itr_create(qn_http_header_ptr hdr)
{
    qn_http_hdr_iterator_ptr new_itr = calloc(1, sizeof(qn_http_hdr_iterator));
    if (!new_itr) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_itr->hdr = hdr;
    return new_itr;
}

void qn_http_hdr_itr_destroy(qn_http_hdr_iterator_ptr itr)
{
    if (itr) {
        free(itr);
    }
}

void qn_http_hdr_itr_rewind(qn_http_hdr_iterator_ptr itr)
{
    itr->pos = 0;
}

qn_bool qn_http_hdr_itr_next_pair_raw(qn_http_hdr_iterator_ptr itr, const char ** key, qn_size * key_size, const char ** val, qn_size * val_size)
{
    qn_string entry;

    if (itr->pos == itr->hdr->cnt) return qn_false;
    entry = itr->hdr->entries[itr->pos++];

    *key = qn_str_cstr(entry);
    *val = strchr(*key, ':') + 2;

    *key_size = *val - *key - 2;
    *val_size = qn_str_size(entry) - *key_size - 2;
    return qn_true;
}

const qn_string qn_http_hdr_itr_next_entry(qn_http_hdr_iterator_ptr itr)
{
    return (itr->pos == itr->hdr->cnt) ? NULL : itr->hdr->entries[itr->pos++];
}

#ifdef __cplusplus
}
#endif

