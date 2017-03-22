#ifndef __QN_HTTP_HEADER_H__
#define __QN_HTTP_HEADER_H__

#include "qiniu/base/string.h"
#include "qiniu/ds/etable.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- HTTP Header Functions (abbreviation: http_hdr) ----

struct _QN_HTTP_HEADER;
typedef struct _QN_HTTP_HEADER * qn_http_header_ptr;

static inline qn_http_header_ptr qn_http_hdr_create(void)
{
    return (qn_http_header_ptr) qn_etbl_create(": ");
}

static inline void qn_http_hdr_destroy(qn_http_header_ptr restrict hdr)
{
    qn_etbl_destroy((qn_etable_ptr) hdr);
}

static inline void qn_http_hdr_reset(qn_http_header_ptr restrict hdr)
{
    qn_etbl_reset((qn_etable_ptr) hdr);
}

static inline int qn_http_hdr_count(qn_http_header_ptr restrict hdr)
{
    return qn_etbl_count((qn_etable_ptr) hdr);
}

static inline qn_string qn_http_hdr_get_entry(qn_http_header_ptr restrict hdr, const char * restrict key)
{
    return qn_etbl_get_entry((qn_etable_ptr) hdr, key);
}

static inline const char * qn_http_hdr_get_value(qn_http_header_ptr restrict hdr, const char * restrict key)
{
    return qn_etbl_get_value((qn_etable_ptr) hdr, key);
}

static inline qn_bool qn_http_hdr_set_raw(qn_http_header_ptr restrict hdr, const char * restrict key, qn_size key_size, const char * restrict val, qn_size val_size)
{
    return qn_etbl_set_raw((qn_etable_ptr) hdr, key, key_size, val, val_size);
}

static inline qn_bool qn_http_hdr_set_text(qn_http_header_ptr restrict hdr, const char * restrict key, const char * restrict val, qn_size val_size)
{
    return qn_http_hdr_set_raw(hdr, key, strlen(key), val, val_size);
}

static inline qn_bool qn_http_hdr_set_string(qn_http_header_ptr restrict hdr, const char * restrict key, const char * restrict val)
{
    return qn_http_hdr_set_raw(hdr, key, strlen(key), val, strlen(val));
}

static inline qn_bool qn_http_hdr_set_integer(qn_http_header_ptr restrict hdr, const char * restrict key, int val)
{
    return qn_etbl_set_integer((qn_etable_ptr) hdr, key, val);
}

static inline void qn_http_hdr_unset(qn_http_header_ptr restrict hdr, const char * restrict key)
{
    qn_etbl_unset((qn_etable_ptr) hdr, key);
}

// ---- HTTP Header Iterator Functions (abbreviation: http_hdr_itr) ----

struct _QN_HTTP_HDR_ITERATOR;
typedef struct _QN_HTTP_HDR_ITERATOR * qn_http_hdr_iterator_ptr;

static inline qn_http_hdr_iterator_ptr qn_http_hdr_itr_create(qn_http_header_ptr restrict hdr)
{
    return (qn_http_hdr_iterator_ptr) qn_etbl_itr_create((qn_etable_ptr) hdr);
}

static inline void qn_http_hdr_itr_destroy(qn_http_hdr_iterator_ptr restrict itr)
{
    qn_etbl_itr_destroy((qn_etbl_iterator_ptr) itr);
}

static inline void qn_http_hdr_itr_rewind(qn_http_hdr_iterator_ptr restrict itr)
{
    qn_etbl_itr_rewind((qn_etbl_iterator_ptr) itr);
}

static inline const qn_string qn_http_hdr_itr_next_entry(qn_http_hdr_iterator_ptr restrict itr)
{
    return qn_etbl_itr_next_entry((qn_etbl_iterator_ptr) itr);
}

static inline qn_bool qn_http_hdr_itr_next_pair_raw(qn_http_hdr_iterator_ptr restrict itr, const char ** restrict key, qn_size * restrict key_size, const char ** restrict val, qn_size * restrict val_size)
{
    return qn_etbl_itr_next_pair_raw((qn_etbl_iterator_ptr) itr, key, key_size, val, val_size);
}

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_HEADER_H__

