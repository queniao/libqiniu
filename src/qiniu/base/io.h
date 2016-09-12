#ifndef __QN_IO_H__
#define __QN_IO_H__

#include "qiniu/base/basic_types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*qn_io_read)(void * restrict user_data, char * restrict buf, int buf_size);
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
    int size;
    int pos;
    qn_io_reader_ptr rdr;
} qn_io_section_reader, *qn_io_section_reader_ptr;

QN_API extern void qn_io_srd_init(qn_io_section_reader_ptr restrict srdr, qn_io_reader_ptr restrict rdr, int size);
QN_API extern int qn_io_srd_read(void * restrict user_data, char * restrict buf, int buf_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_IO_H__

