#include <openssl/sha.h>

#include "qiniu/base/errors.h"
#include "qiniu/etag.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of QETAG calculator

typedef unsigned int qn_etag_alloc;
typedef unsigned short int qn_etag_pos;

#define QN_ETAG_BLK_MAX_SIZE (1 << 22)

#if (!defined(QN_ETAG_BLK_MAX_COUNT) || QN_ETAG_BLK_MAX_COUNT < 2)
#undef QN_ETAG_BLK_MAX_COUNT
#define QN_ETAG_BLK_MAX_COUNT 2
#endif

#define QN_ETAG_ALLOCATION_MAX_BITS (sizeof(qn_etag_alloc) * 8)
#define QN_ETAG_ALLOCATION_MAX_COUNT ((int)((QN_ETAG_BLK_MAX_COUNT + QN_ETAG_ALLOCATION_MAX_BITS - 1) / QN_ETAG_ALLOCATION_MAX_BITS))

#define QN_ETAG_ALLOC_RESET(alloc, num) (alloc[(int)(num / QN_ETAG_ALLOCATION_MAX_BITS)] &= ~((qn_etag_alloc)0x1L << (num % QN_ETAG_ALLOCATION_MAX_BITS)))
#define QN_ETAG_ALLOC_SET(alloc, num) (alloc[(int)(num / QN_ETAG_ALLOCATION_MAX_BITS)] |= ((qn_etag_alloc)0x1L << (num % QN_ETAG_ALLOCATION_MAX_BITS)))
#define QN_ETAG_ALLOC_IS_SET(alloc, num) (alloc[(int)(num / QN_ETAG_ALLOCATION_MAX_BITS)] & ((qn_etag_alloc)0x1L << (num % QN_ETAG_ALLOCATION_MAX_BITS)))

typedef struct _QN_ETAG_BLOCK
{
    SHA_CTX sha1_ctx; 
} qn_etag_block;

QN_SDK qn_bool qn_etag_blk_update(qn_etag_block_ptr restrict blk, char * restrict buf, int buf_size)
{
    if (SHA1_Update(&blk->sha1_ctx, buf, buf_size) == 0) {
        qn_err_etag_set_updating_block_failed();
        return qn_false;
    } // if
    return qn_true;
}

// ----

enum
{
    QN_ETAG_FLAG_MULTI_BLOCKS = 0x1
};

typedef struct _QN_ETAG_CONTEXT
{
    qn_etag_pos unused;
    qn_etag_pos begin;
    qn_etag_pos end;
    qn_etag_pos flags;

    qn_etag_block_ptr blk;
    int blk_cap;

    qn_etag_alloc allocs[QN_ETAG_ALLOCATION_MAX_COUNT];
    qn_etag_block blks[QN_ETAG_BLK_MAX_COUNT];

    SHA_CTX sha1_ctx;
} qn_etag_context;

QN_SDK qn_etag_context_ptr qn_etag_ctx_create(void)
{
    qn_etag_context_ptr new_ctx = calloc(1, sizeof(qn_etag_context));
    if (!new_ctx) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    if (!qn_etag_ctx_init(new_ctx)) {
        free(new_ctx);
        return NULL;
    } // if
    return new_ctx;
}

QN_SDK void qn_etag_ctx_destroy(qn_etag_context_ptr restrict ctx)
{
    if (ctx) {
        free(ctx);
    } // if
}

static qn_bool qn_etag_ctx_merge_blocks(qn_etag_context_ptr ctx)
{
    unsigned char digest[SHA_DIGEST_LENGTH];
    qn_etag_pos i = ctx->begin / QN_ETAG_BLK_MAX_COUNT;

    qn_etag_alloc b = 0x1L << (ctx->begin % QN_ETAG_BLK_MAX_COUNT);
    while (ctx->unused < QN_ETAG_BLK_MAX_COUNT && ctx->allocs[i] & b) {
        if (SHA1_Final(digest, &ctx->blks[ctx->begin].sha1_ctx) == 0) {
            qn_err_etag_set_making_digest_failed();
            return qn_false;
        } // if
        if (SHA1_Update(&ctx->sha1_ctx, digest, sizeof(digest)) == 0) {
            qn_err_etag_set_updating_context_failed();
            return qn_false;
        } // if

        ctx->allocs[i] &= ~b; // reset
        if (b == 0) {
            b = 0x1L;
            i += 1;
            if (i == QN_ETAG_ALLOCATION_MAX_COUNT) i = 0;
        } else {
            b <<= 1;
        } // if
        if (++ctx->begin == QN_ETAG_BLK_MAX_COUNT) ctx->begin = 0;
        ctx->unused += 1;
    } // while
    return qn_true;
}

QN_SDK qn_bool qn_etag_ctx_init(qn_etag_context_ptr restrict ctx)
{
    if (SHA1_Init(&ctx->sha1_ctx) == 0) {
        qn_err_etag_set_initializing_context_failed();
        return qn_false;
    } // if
    if (SHA1_Init(&ctx->blks[0].sha1_ctx) == 0) {
        qn_err_etag_set_initializing_context_failed();
        return qn_false;
    } // if
    ctx->unused = QN_ETAG_BLK_MAX_COUNT;
    ctx->begin = 0;
    ctx->end = 0;
    ctx->flags = 0;
    ctx->blk = NULL;
    ctx->blk_cap = 0;
    return qn_true;
}

QN_SDK qn_bool qn_etag_ctx_update(qn_etag_context_ptr restrict ctx, char * restrict buf, int buf_size)
{
    int update_size;
    int rem_size = buf_size;
    char * pos = buf;

    while (rem_size > 0) {
        if (!ctx->blk) {
            qn_etag_ctx_allocate_block(ctx, &ctx->blk, &ctx->blk_cap);
        } // if

        update_size = ctx->blk_cap < rem_size ? ctx->blk_cap : rem_size;

        if (!qn_etag_blk_update(ctx->blk, pos, update_size)) return qn_false;
        rem_size -= update_size;
        pos += update_size;

        ctx->blk_cap -= update_size;
        if (ctx->blk_cap == 0) {
            if (!qn_etag_ctx_commit_block(ctx, ctx->blk)) return qn_false;
            ctx->blk = NULL;
        } // if
    } // while
    return qn_true;
}

QN_SDK qn_string qn_etag_ctx_final(qn_etag_context_ptr restrict ctx)
{
    unsigned char digest_data[4 + SHA_DIGEST_LENGTH];

    if (ctx->blk) {
        if (!qn_etag_ctx_commit_block(ctx, ctx->blk)) return NULL;
        ctx->blk = NULL;
    } // if

    if ((ctx->flags & QN_ETAG_FLAG_MULTI_BLOCKS) == 0) {
        // One block case
        digest_data[0] = 0x16;
        if (SHA1_Final(&digest_data[1], &ctx->blks[0].sha1_ctx) == 0) {
            qn_err_etag_set_making_digest_failed();
            return NULL;
        } // if
    } else {
        // Multi-blocks case
        if (ctx->unused < QN_ETAG_BLK_MAX_COUNT) return NULL;

        digest_data[0] = 0x96;
        if (SHA1_Final(&digest_data[1], &ctx->sha1_ctx) == 0) {
            qn_err_etag_set_making_digest_failed();
            return NULL;
        } // if
    } // if

    return qn_cs_encode_base64_urlsafe((char *)digest_data, 1 + SHA_DIGEST_LENGTH);
}

QN_SDK qn_bool qn_etag_ctx_allocate_block(qn_etag_context_ptr restrict ctx, qn_etag_block_ptr * restrict blk, int * restrict buf_cap)
{
    if (ctx->unused == 0) return qn_false;

    if (SHA1_Init(&ctx->blks[ctx->end].sha1_ctx) == 0) {
        qn_err_etag_set_initializing_block_failed();
        return qn_false;
    } // if
    
    *blk = &ctx->blks[ctx->end];
    QN_ETAG_ALLOC_RESET(ctx->allocs, ctx->end);
    if (++ctx->end >= QN_ETAG_BLK_MAX_COUNT) {
        ctx->end = 0;
    } // if

    ctx->unused -= 1;
    *buf_cap = QN_ETAG_BLK_MAX_SIZE;
    return qn_true;
}

QN_SDK qn_bool qn_etag_ctx_commit_block(qn_etag_context_ptr restrict ctx, qn_etag_block_ptr blk)
{
    int pos = blk - &ctx->blks[0];
    QN_ETAG_ALLOC_SET(ctx->allocs, pos);
    if ((ctx->flags & QN_ETAG_FLAG_MULTI_BLOCKS) == 0) {
        if (ctx->unused == QN_ETAG_BLK_MAX_COUNT - 1 && pos == 0) {
            return qn_true;
        } else {
            ctx->flags |= QN_ETAG_FLAG_MULTI_BLOCKS;
        } // if
    } // if
    return qn_etag_ctx_merge_blocks(ctx);
}

// ----

QN_SDK extern qn_string qn_etag_digest_file(const char * restrict fname);

QN_SDK qn_string qn_etag_digest_buffer(char * restrict buf, int buf_size)
{
    qn_string digest;
    qn_etag_context_ptr ctx = qn_etag_ctx_create();
    if (!ctx) return NULL;

    if (!qn_etag_ctx_update(ctx, buf, buf_size)) return NULL;
    digest = qn_etag_ctx_final(ctx);
    qn_etag_ctx_destroy(ctx);
    return digest;
}

#ifdef __cplusplus
}
#endif
