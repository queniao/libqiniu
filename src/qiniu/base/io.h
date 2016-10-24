#ifndef __QN_IO_H__
#define __QN_IO_H__

#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef ssize_t (*qn_io_read)(void * restrict user_data, char * restrict buf, size_t buf_size);
typedef qn_bool (*qn_io_advance)(void * restrict user_data, int delta);

typedef struct _QN_IO_READER
{
    void * user_data;
    qn_io_read read;
    qn_io_advance advance;
} qn_io_reader, *qn_io_reader_ptr;

// ----

typedef struct _QN_IO_SECTION_READER
{
    size_t size;
    int pos;
    qn_io_reader_ptr rdr;
} qn_io_section_reader, *qn_io_section_reader_ptr;

QN_API extern void qn_io_srd_init(qn_io_section_reader_ptr restrict srdr, qn_io_reader_ptr restrict rdr, size_t size);
QN_API extern ssize_t qn_io_srd_read(void * restrict user_data, char * restrict buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_IO_H__

