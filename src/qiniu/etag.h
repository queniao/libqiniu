#ifndef __QN_ETAG_H__
#define __QN_ETAG_H__

#include "qiniu/base/basic_types.h"
#include "qiniu/base/string.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of QETAG calculator

struct _QN_ETAG_BLOCK;
typedef struct _QN_ETAG_BLOCK * qn_etag_block_ptr;

extern qn_bool qn_etag_blk_update(qn_etag_block_ptr blk, char * buf, int buf_size);

// ----

struct _QN_ETAG_CONTEXT;
typedef struct _QN_ETAG_CONTEXT * qn_etag_context_ptr;

extern qn_etag_context_ptr qn_etag_ctx_create(void);
extern void qn_etag_ctx_destroy(qn_etag_context_ptr ctx);

extern qn_bool qn_etag_ctx_init(qn_etag_context_ptr ctx);
extern qn_bool qn_etag_ctx_update(qn_etag_context_ptr ctx, char * buf, int buf_size);
extern qn_string qn_etag_ctx_final(qn_etag_context_ptr ctx);

extern qn_bool qn_etag_ctx_allocate_block(qn_etag_context_ptr ctx, qn_etag_block_ptr * blk, int * buf_cap);
extern qn_bool qn_etag_ctx_commit_block(qn_etag_context_ptr ctx, qn_etag_block_ptr blk);

// ----

extern qn_string qn_etag_digest_file(const char * fname);
extern qn_string qn_etag_digest_buffer(char * buf, int buf_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_ETAG_H__
