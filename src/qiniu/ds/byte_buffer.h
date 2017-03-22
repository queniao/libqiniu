#ifndef __QN_DS_BYTE_BUFFER_H__
#define __QN_DS_BYTE_BUFFER_H__

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct _QN_BYTE_BUFFER;
typedef struct _QN_BYTE_BUFFER * qn_byte_buffer_ptr;

QN_SDK extern qn_byte_buffer_ptr qn_bb_create(qn_size init_size);
QN_SDK extern void qn_bb_destroy(qn_byte_buffer_ptr restrict bb);
QN_SDK extern void qn_bb_reset(qn_byte_buffer_ptr restrict bb);

QN_SDK extern qn_size qn_bb_size(qn_byte_buffer_ptr restrict bb);

QN_SDK extern qn_bool qn_bb_append_binary(qn_byte_buffer_ptr restrict bb, const char * restrict bin, qn_size bin_size);

static inline qn_bool qn_bb_append_string(qn_byte_buffer_ptr restrict bb, qn_string restrict str)
{
    return qn_bb_append_binary(bb, qn_str_cstr(str), qn_str_size(str));
}

static inline qn_bool qn_bb_append_cstr(qn_byte_buffer_ptr restrict bb, const char * restrict str)
{
    return qn_bb_append_binary(bb, qn_str_cstr(str), posix_strlen(str));
}

static inline qn_bool qn_bb_append_text(qn_byte_buffer_ptr restrict bb, const char * restrict str, qn_size str_size)
{
    return qn_bb_append_binary(bb, str, str_size);
}

QN_SDK extern const char * qn_bb_to_cstr(qn_byte_buffer_ptr restrict bb);

#ifdef __cplusplus
}
#endif

#endif // __QN_DS_BYTE_BUFFER_H__

