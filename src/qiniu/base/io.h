#ifndef __QN_IO_H__
#define __QN_IO_H__

#include "qiniu/os/types.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum
{
    QN_IO_RDR_FILTERING_FAILED = -3,
    QN_IO_RDR_READING_ABORTED = -2,
    QN_IO_RDR_READING_FAILED = -1,
    QN_IO_RDR_EOF = 0
};

struct _QN_IO_READER;
typedef struct _QN_IO_READER * qn_io_reader_ptr;

typedef void (*qn_io_close_fn)(void * restrict user_data);
typedef ssize_t (*qn_io_peek_fn)(void * restrict user_data, char * restrict buf, size_t buf_size);
typedef ssize_t (*qn_io_read_fn)(void * restrict user_data, char * restrict buf, size_t buf_size);
typedef qn_bool (*qn_io_seek_fn)(void * restrict user_data, qn_fsize offset);
typedef qn_bool (*qn_io_advance_fn)(void * restrict user_data, size_t delta);

typedef qn_io_reader_ptr (*qn_io_duplicate_fn)(void * restrict user_data);
typedef qn_io_reader_ptr (*qn_io_section_fn)(void * restrict user_data, qn_fsize offset, size_t sec_size);

typedef struct _QN_IO_READER
{
    qn_io_close_fn close;
    qn_io_peek_fn peek;
    qn_io_read_fn read;
    qn_io_seek_fn seek;
    qn_io_advance_fn advance;

    qn_io_duplicate_fn duplicate;
    qn_io_section_fn section;
} qn_io_reader;

#ifdef __cplusplus
}
#endif

#endif // __QN_IO_H__

