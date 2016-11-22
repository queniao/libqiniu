#include <stdlib.h>

#include "qiniu/base/io.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_IO_SECTION_READER
{
    qn_io_reader vtbl;
    qn_io_reader_ptr src_rdr;
    size_t size;
    size_t pos;
} qn_io_section_reader;

static qn_io_reader qn_io_srdr_vtable = {
    (qn_io_read_fn) &qn_io_srdr_read,
    (qn_io_advance_fn) &qn_io_srdr_advance
};

static inline void qn_io_srdr_init(qn_io_section_reader_ptr restrict srdr, qn_io_reader_ptr restrict src_rdr, size_t size)
{
    srdr->vtbl = qn_io_srdr_vtable;
    srdr->src_rdr = src_rdr;
    srdr->size = size;
    srdr->pos = 0;
}

QN_API qn_io_section_reader_ptr qn_io_srdr_create(qn_io_reader_ptr restrict src_rdr, size_t sec_size)
{
    qn_io_section_reader_ptr new_srdr = calloc(1, sizeof(qn_io_section_reader));
    if (!new_srdr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    qn_io_srdr_init(new_srdr, src_rdr, sec_size);
    return new_srdr;
}

QN_API void qn_io_srdr_destroy(qn_io_section_reader_ptr restrict srdr)
{
    if (srdr) {
        free(srdr);
    } // if
}

QN_API qn_io_reader_ptr qn_io_srdr_to_io_reader(qn_io_section_reader_ptr restrict srdr)
{
    return &srdr->vtbl;
}

QN_API ssize_t qn_io_srdr_read(qn_io_section_reader_ptr restrict srdr, char * restrict buf, size_t buf_size)
{
    size_t reading_bytes;
    int ret;

    reading_bytes = srdr->size - srdr->pos;
    if (reading_bytes > buf_size) reading_bytes = buf_size;

    ret = srdr->src_rdr->read(srdr->src_rdr, buf, reading_bytes);
    if (ret < 0) return -1;

    srdr->pos += ret;
    return ret;
}

QN_API qn_bool qn_io_srdr_advance(qn_io_section_reader_ptr restrict srdr, size_t delta)
{
    return srdr->src_rdr->advance(srdr->src_rdr, delta);
}

#ifdef __cplusplus
}
#endif
