#include "qiniu/reader_filter.h"
#include "qiniu/reader.h"
#include "qiniu/etag.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of ETAG Filter ----

struct _QN_FLT_ETAG
{
    qn_etag_context_ptr ctx;
} qn_flt_etag;

QN_API qn_flt_etag_ptr qn_flt_etag_create(void)
{
    qn_etag_context_ptr ctx = qn_etag_ctx_create();
    if (!ctx) return NULL;
    return (qn_flt_etag_ptr) ctx;
}

QN_API void qn_flt_etag_destroy(qn_flt_etag_ptr restrict etag)
{
    if (etag) {
        qn_etag_ctx_destroy((qn_etag_context_ptr) etag);
    } // if
}

QN_API qn_bool qn_flt_etag_reset(qn_flt_etag_ptr restrict etag)
{
    return qn_etag_ctx_init((qn_etag_context_ptr) etag);
}

QN_API qn_string qn_flt_etag_result(qn_flt_etag_ptr restrict etag)
{
    return qn_etag_ctx_final((qn_etag_context_ptr) etag);
}

QN_API ssize_t qn_flt_etag_callback(void * restrict user_data, char ** restrict buf, size_t * restrict size)
{
    if (!qn_etag_ctx_update((qn_etag_context_ptr) user_data, *buf, *size)) return QN_RDR_FILTERING_FAILED;
    return QN_RDR_SUCCEED;
}

#ifdef __cplusplus
}
#endif

