#ifndef __QN_READER_H__
#define __QN_READER_H__ 1

#include "qiniu/base/io.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum
{
    QN_RDR_READING_ABORTED = -2,
    QN_RDR_READING_FAILED = -1,
    QN_RDR_SUCCEED = 0
};

typedef short int qn_rdr_pos;

typedef ssize_t (*qn_rdr_filter_callback)(void * restrict user_data, char ** restrict buf, size_t * restrict size);

struct _QN_READER;
typedef struct _QN_READER * qn_reader_ptr;

QN_API extern qn_reader_ptr qn_rdr_create(qn_io_reader_ptr src_rdr, qn_rdr_pos filter_num);
QN_API extern void qn_rdr_destroy(qn_reader_ptr restrict rdr);
QN_API extern void qn_rdr_reset(qn_reader_ptr restrict rdr);

QN_API extern qn_bool qn_rdr_add_pre_filter(qn_reader_ptr restrict rdr, void * restrict filter_data, qn_rdr_filter_callback filter_cb);
QN_API extern qn_bool qn_rdr_add_post_filter(qn_reader_ptr restrict rdr, void * restrict filter_data, qn_rdr_filter_callback filter_cb);

QN_API extern ssize_t qn_rdr_read(qn_reader_ptr restrict rdr, char * restrict buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif // __QN_READER_H__

