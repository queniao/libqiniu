#ifndef __QN_IO_H__
#define __QN_IO_H__

#include "qiniu/base/basic_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*qn_io_read)(void * user_data, char * buf, int buf_size);
typedef qn_bool (*qn_io_advance)(void * user_data, int delta);

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

extern void qn_io_srd_init(qn_io_section_reader_ptr srdr, qn_io_reader_ptr rdr, int size);
extern int qn_io_srd_read(void * user_data, char * buf, int buf_size);

#ifdef __cplusplus
}
#endif

#endif // __QN_IO_H__

