#ifndef __QN_HTTP_QUERY_H__
#define __QN_HTTP_QUERY_H__

#include "qiniu/ds/etable.h"
#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- HTTP Header Functions (abbreviation: http_hdr) ----

struct _QN_HTTP_QUERY;
typedef struct _QN_HTTP_QUERY * qn_http_query_ptr;

static inline qn_http_query_ptr qn_http_qry_create(void)
{
    return (qn_http_query_ptr) qn_etbl_create("=");
}

static inline void qn_http_qry_destroy(qn_http_query_ptr restrict qry)
{
    qn_etbl_destroy((qn_etable_ptr) qry);
}

static inline void qn_http_qry_reset(qn_http_query_ptr restrict qry)
{
    qn_etbl_reset((qn_etable_ptr) qry); 
}

static inline int qn_http_qry_count(qn_http_query_ptr restrict qry)
{
    return qn_etbl_count((qn_etable_ptr) qry);
}

static inline qn_bool qn_http_qry_set_raw(qn_http_query_ptr restrict qry, const char * restrict key, qn_size key_size, const char * restrict val, qn_size val_size)
{
    qn_bool ret;
    qn_string encoded_val = qn_cs_percent_encode(val, val_size);
    ret = qn_etbl_set_raw((qn_etable_ptr) qry, key, key_size, qn_str_cstr(encoded_val), qn_str_size(encoded_val));
    qn_str_destroy(encoded_val);
    return ret;
}

static inline qn_bool qn_http_qry_set_text(qn_http_query_ptr restrict qry, const char * restrict key, const char * restrict val, qn_size val_size)
{
    return qn_http_qry_set_raw(qry, key, strlen(key), val, val_size);
}

static inline qn_bool qn_http_qry_set_string(qn_http_query_ptr restrict qry, const char * restrict key, const char * restrict val)
{
    return qn_http_qry_set_raw(qry, key, strlen(key), val, strlen(val));
}

static inline qn_bool qn_http_qry_set_integer(qn_http_query_ptr restrict qry, const char * restrict key, int value)
{
    return qn_etbl_set_integer((qn_etable_ptr) qry, key, value);
}

static inline qn_string qn_http_qry_to_string(qn_http_query_ptr restrict qry)
{
    return qn_str_join_list("&", qn_etbl_entries((qn_etable_ptr) qry), qn_etbl_count((qn_etable_ptr) qry));
}

// ---- HTTP Query Iterator Functions (abbreviation: http_hdr_itr) ----

struct _QN_HTTP_QRY_ITERATOR;
typedef struct _QN_HTTP_QRY_ITERATOR * qn_http_qry_iterator_ptr;

static inline qn_http_qry_iterator_ptr qn_http_qry_itr_create(qn_http_query_ptr restrict qry)
{
    return (qn_http_qry_iterator_ptr) qn_etbl_itr_create((qn_etable_ptr) qry);
}

static inline void qn_http_qry_itr_destroy(qn_http_qry_iterator_ptr restrict itr)
{
    qn_etbl_itr_destroy((qn_etbl_iterator_ptr) itr);
}

static inline void qn_http_qry_itr_rewind(qn_http_qry_iterator_ptr restrict itr)
{
    qn_etbl_itr_rewind((qn_etbl_iterator_ptr) itr);
}

static inline const qn_string qn_http_qry_itr_next_entry(qn_http_qry_iterator_ptr restrict itr)
{
    return qn_etbl_itr_next_entry((qn_etbl_iterator_ptr) itr);
}

static inline qn_bool qn_http_qry_itr_next_pair_raw(qn_http_qry_iterator_ptr restrict itr, const char ** restrict key, qn_size * restrict key_size, const char ** restrict val, qn_size * restrict val_size)
{
    return qn_etbl_itr_next_pair_raw((qn_etbl_iterator_ptr) itr, key, key_size, val, val_size);
}

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_QUERY_H__
