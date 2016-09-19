#ifndef __QN_HTTP_QUERY_H__
#define __QN_HTTP_QUERY_H__

#include "qiniu/base/ds/etable.h"
#include "qiniu/base/string.h"
#include "qiniu/macros.h"

// ---- Declaration of HTTP query ----

#ifdef __cplusplus
extern "C"
{
#endif

struct _QN_HTTP_QUERY;
typedef struct _QN_HTTP_QUERY * qn_http_query_ptr;

QN_API extern qn_http_query_ptr qn_http_qry_create(void);
QN_API extern void qn_http_qry_destroy(qn_http_query_ptr restrict qry);
QN_API extern void qn_http_qry_reset(qn_http_query_ptr restrict qry);

QN_API extern int qn_http_qry_count(qn_http_query_ptr restrict qry);

QN_API extern qn_bool qn_http_qry_set_raw(qn_http_query_ptr restrict qry, const char * restrict key, size_t key_size, const char * restrict val, size_t val_size);

static inline qn_bool qn_http_qry_set_text(qn_http_query_ptr restrict qry, const char * restrict key, const char * restrict val, size_t val_size)
{
    return qn_http_qry_set_raw(qry, key, strlen(key), val, val_size);
}

static inline qn_bool qn_http_qry_set_string(qn_http_query_ptr restrict qry, const char * restrict key, const char * restrict val)
{
    return qn_http_qry_set_raw(qry, key, strlen(key), val, strlen(val));
}

QN_API extern qn_bool qn_http_qry_set_integer(qn_http_query_ptr restrict qry, const char * restrict key, int value);

QN_API extern qn_string qn_http_qry_to_string(qn_http_query_ptr restrict qry);

#ifdef __cplusplus
}
#endif

#endif // __QN_HTTP_QUERY_H__
