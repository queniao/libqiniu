#ifndef __QN_ETAG_H__
#define __QN_ETAG_H__ 1

#include "qiniu/base/string.h"
#include "qiniu/os/types.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of QETAG calculator

struct _QN_ETAG_BLOCK;
typedef struct _QN_ETAG_BLOCK * qn_etag_block_ptr;

QN_API extern qn_bool qn_etag_blk_update(qn_etag_block_ptr restrict blk, char * restrict buf, int buf_size);

// ----

struct _QN_ETAG_CONTEXT;
typedef struct _QN_ETAG_CONTEXT * qn_etag_context_ptr;

QN_API extern qn_etag_context_ptr qn_etag_ctx_create(void);
QN_API extern void qn_etag_ctx_destroy(qn_etag_context_ptr restrict ctx);

QN_API extern qn_bool qn_etag_ctx_init(qn_etag_context_ptr restrict ctx);
QN_API extern qn_bool qn_etag_ctx_update(qn_etag_context_ptr restrict ctx, char * restrict buf, int buf_size);
QN_API extern qn_string qn_etag_ctx_final(qn_etag_context_ptr restrict ctx);

QN_API extern qn_bool qn_etag_ctx_allocate_block(qn_etag_context_ptr restrict ctx, qn_etag_block_ptr * restrict blk, int * restrict buf_cap);
QN_API extern qn_bool qn_etag_ctx_commit_block(qn_etag_context_ptr restrict ctx, qn_etag_block_ptr blk);

// ----

QN_API extern qn_string qn_etag_digest_file(const char * restrict fname);
QN_API extern qn_string qn_etag_digest_buffer(char * restrict buf, int buf_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_ETAG_H__

