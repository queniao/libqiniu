#ifndef __QN_HTTP_HEADER_H__
#define __QN_HTTP_HEADER_H__

#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of HTTP header ----

struct _QN_HTTP_HEADER;
typedef struct _QN_HTTP_HEADER * qn_http_header_ptr;

QN_API extern qn_http_header_ptr qn_http_hdr_create(void);
QN_API extern void qn_http_hdr_destroy(qn_http_header_ptr restrict hdr);
QN_API extern void qn_http_hdr_reset(qn_http_header_ptr restrict hdr);

QN_API extern int qn_http_hdr_count(qn_http_header_ptr restrict hdr);

QN_API extern qn_string qn_http_hdr_get_entry(qn_http_header_ptr restrict hdr, const char * restrict key);
QN_API extern const char * qn_http_hdr_get_value(qn_http_header_ptr restrict hdr, const char * restrict key);

QN_API extern qn_bool qn_http_hdr_set_raw(qn_http_header_ptr restrict hdr, const char * restrict key, size_t key_size, const char * restrict val, size_t val_size);

static inline qn_bool qn_http_hdr_set_text(qn_http_header_ptr restrict hdr, const char * restrict key, const char * restrict val, size_t val_size)
{
    return qn_http_hdr_set_raw(hdr, key, strlen(key), val, val_size);
}

static inline qn_bool qn_http_hdr_set_string(qn_http_header_ptr restrict hdr, const char * restrict key, const char * restrict val)
{
    return qn_http_hdr_set_raw(hdr, key, strlen(key), val, strlen(val));
}

QN_API extern qn_bool qn_http_hdr_set_integer(qn_http_header_ptr restrict hdr, const char * restrict key, int val);
QN_API extern void qn_http_hdr_unset(qn_http_header_ptr restrict hdr, const char * restrict key);

// ---- Declaration of HTTP header iterator ----

typedef void * qn_http_hdr_iterator_ptr;

QN_API extern qn_http_hdr_iterator_ptr qn_http_hdr_itr_create(qn_http_header_ptr restrict hdr);
QN_API extern void qn_http_hdr_itr_destroy(qn_http_hdr_iterator_ptr restrict itr);
QN_API extern void qn_http_hdr_itr_rewind(qn_http_hdr_iterator_ptr restrict itr);

QN_API extern const qn_string qn_http_hdr_itr_next_entry(qn_http_hdr_iterator_ptr restrict itr);
QN_API extern qn_bool qn_http_hdr_itr_next_pair_raw(qn_http_hdr_iterator_ptr restrict itr, const char ** restrict key, size_t * restrict key_size, const char ** restrict val, size_t * restrict val_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_HEADER_H__

