#ifndef __QN_IO_H__
#define __QN_IO_H__

#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef ssize_t (*qn_io_read_fn)(void * restrict user_data, char * restrict buf, size_t buf_size);
typedef qn_bool (*qn_io_advance_fn)(void * restrict user_data, size_t delta);

typedef struct _QN_IO_READER
{
    qn_io_read_fn read;
    qn_io_advance_fn advance;
} qn_io_reader, *qn_io_reader_ptr;

// ----

struct _QN_IO_SECTION_READER;
typedef struct _QN_IO_SECTION_READER * qn_io_section_reader_ptr;

QN_API extern qn_io_section_reader_ptr qn_io_srdr_create(qn_io_reader_ptr restrict src_rdr, size_t sec_size);
QN_API extern void qn_io_srdr_destroy(qn_io_section_reader_ptr restrict srdr);

QN_API extern qn_io_reader_ptr qn_io_srdr_to_io_reader(qn_io_section_reader_ptr restrict srdr);

QN_API extern ssize_t qn_io_srdr_read(qn_io_section_reader_ptr restrict srdr, char * restrict buf, size_t buf_size);
QN_API extern qn_bool qn_io_srdr_advance(qn_io_section_reader_ptr restrict srdr, size_t delta);

#ifdef __cplusplus
}
#endif

#endif // __QN_IO_H__

