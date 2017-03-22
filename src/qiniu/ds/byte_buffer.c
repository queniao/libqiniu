#include "qiniu/ds/byte_buffer.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_BYTE_BUFFER
{
    char * buf;
    qn_size cnt;
    qn_size cap;
} qn_byte_buffer_st;

QN_SDK qn_byte_buffer_ptr qn_bb_create(qn_size init_size)
{
    qn_byte_buffer_ptr new_buf = calloc(1, sizeof(qn_byte_buffer_st));
    if (! new_buf) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    if (0 < init_size) {
        new_buf->buf = malloc(init_size);
        if (! new_buf->buf) {
            free(new_buf);
            qn_err_set_out_of_memory();
            return NULL;
        } // if
    } // if

    new_buf->cnt = 0;
    new_buf->cap = init_size;
    return new_buf;
}

QN_SDK void qn_bb_destroy(qn_byte_buffer_ptr restrict bb)
{
    if (bb) {
        free(bb->buf);
        free(bb);
    } // if
}

QN_SDK void qn_bb_reset(qn_byte_buffer_ptr restrict bb)
{
    bb->cnt = 0;
}

QN_SDK qn_size qn_bb_size(qn_byte_buffer_ptr restrict bb)
{
    return bb->cnt;
}

static qn_bool qn_bb_augment(qn_byte_buffer_ptr restrict bb)
{
    qn_size new_cap = (0 < bb->cap) ? (bb->cap + (bb->cap >> 1)) : 256;
    char * new_buf = malloc(new_cap);
    if (! new_buf) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_buf, bb->buf, bb->cnt);
    free(bb->buf);

    bb->buf = new_buf;
    bb->cap = new_cap;
    return qn_true;
}

QN_SDK qn_bool qn_bb_append_binary(qn_byte_buffer_ptr restrict bb, const char * restrict bin, qn_size bin_size)
{
    if (bb->cap - bb->cnt < bin_size && ! qn_bb_augment(bb)) return qn_false;
    memcpy(bb->buf + bb->cnt, bin, bin_size);
    bb->cnt += bin_size;
    return qn_true;
}

QN_SDK const char * qn_bb_to_cstr(qn_byte_buffer_ptr restrict bb)
{
    bb->buf[bb->cnt] = '\0';
    return bb->buf;
}

#ifdef __cplusplus
}
#endif
