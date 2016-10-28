#ifndef __QN_READER_FILTER_H__
#define __QN_READER_FILTER_H__ 1

#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of ETAG Filter ----

struct _QN_FLT_ETAG;
typedef struct _QN_FLT_ETAG * qn_flt_etag_ptr;

QN_API extern qn_flt_etag_ptr qn_flt_etag_create(void);
QN_API extern void qn_flt_etag_destroy(qn_flt_etag_ptr restrict etag);
QN_API extern qn_bool qn_flt_etag_reset(qn_flt_etag_ptr restrict etag);

QN_API extern qn_string qn_flt_etag_result(qn_flt_etag_ptr restrict etag);
QN_API extern ssize_t qn_flt_etag_callback(void * restrict user_data, char ** restrict buf, size_t * restrict size);

#ifdef __cplusplus
}
#endif

#endif // __QN_READER_FILTER_H__

