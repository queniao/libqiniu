#ifndef __QN_READER_H__
#define __QN_READER_H__ 1

#include "qiniu/base/io.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef short int qn_rdr_pos;

typedef ssize_t (*qn_rdr_filter_callback_fn)(void * restrict user_data, char ** restrict buf, size_t * restrict size);

struct _QN_READER;
typedef struct _QN_READER * qn_reader_ptr;

QN_SDK extern qn_reader_ptr qn_rdr_create(qn_io_reader_itf src_rdr, qn_rdr_pos filter_num);
QN_SDK extern void qn_rdr_destroy(qn_reader_ptr restrict rdr);
QN_SDK extern void qn_rdr_reset(qn_reader_ptr restrict rdr);

QN_SDK extern qn_reader_ptr qn_rdr_duplicate(qn_reader_ptr restrict rdr);
QN_SDK extern qn_reader_ptr qn_rdr_section(qn_reader_ptr restrict rdr, qn_foffset offset, size_t sec_size);

QN_SDK extern qn_string qn_rdr_name(qn_reader_ptr restrict rdr);
QN_SDK extern qn_fsize qn_rdr_size(qn_reader_ptr restrict rdr);

QN_SDK extern qn_io_reader_itf qn_rdr_to_io_reader(qn_reader_ptr restrict rdr);

QN_SDK extern qn_bool qn_rdr_add_pre_filter(qn_reader_ptr restrict rdr, void * restrict filter_data, qn_rdr_filter_callback_fn filter_cb);
QN_SDK extern qn_bool qn_rdr_add_post_filter(qn_reader_ptr restrict rdr, void * restrict filter_data, qn_rdr_filter_callback_fn filter_cb);

QN_SDK extern ssize_t qn_rdr_peek(qn_reader_ptr restrict rdr, char * restrict buf, size_t size);
QN_SDK extern ssize_t qn_rdr_read(qn_reader_ptr restrict rdr, char * restrict buf, size_t size);

QN_SDK extern qn_bool qn_rdr_seek(qn_reader_ptr restrict rdr, qn_foffset offset);
QN_SDK extern qn_bool qn_rdr_advance(qn_reader_ptr restrict rdr, qn_foffset delta);

#ifdef __cplusplus
}
#endif

#endif // __QN_READER_H__

