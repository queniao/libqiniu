#include <assert.h>
#include <stdlib.h>

#include "qiniu/base/io.h"
#include "qiniu/base/errors.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_IO_SECTION_READER
{
    qn_io_reader_ptr rdr_vtbl;
    qn_io_reader_itf src_rdr;
    size_t rem_size;
    size_t sec_size;
} qn_io_section_reader_st;

static inline qn_io_section_reader_ptr qn_io_srdr_from_io_reader(qn_io_reader_itf restrict itf)
{
    return (qn_io_section_reader_ptr)( ( (char *) itf ) - (char *)( &((qn_io_section_reader_ptr)0)->rdr_vtbl ) );
}

static void qn_io_srdr_close_vfn(qn_io_reader_itf restrict itf)
{
    qn_io_srdr_destroy(qn_io_srdr_from_io_reader(itf));
}

static ssize_t qn_io_srdr_peek_vfn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_io_peek((qn_io_srdr_from_io_reader(itf))->src_rdr, buf, buf_size);
}

static ssize_t qn_io_srdr_read_vfn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    ssize_t ret;
    qn_io_section_reader_ptr srdr = qn_io_srdr_from_io_reader(itf);
    if (srdr->rem_size == 0) return QN_IO_RDR_EOF;

    if (srdr->rem_size < buf_size) {
        ret = qn_io_read(srdr->src_rdr, buf, srdr->rem_size);
    } else {
        ret = qn_io_read(srdr->src_rdr, buf, buf_size);
    } // if
    if (0 < ret) srdr->rem_size -= ret;
    return ret;
}

static qn_bool qn_io_srdr_seek_vfn(qn_io_reader_itf restrict itf, qn_fsize offset)
{
    return qn_true;
}

static qn_bool qn_io_srdr_advance_vfn(qn_io_reader_itf restrict itf, size_t delta)
{
    qn_bool ret;
    qn_io_section_reader_ptr srdr = qn_io_srdr_from_io_reader(itf);
    if (srdr->rem_size == 0) return qn_true;
    
    if (srdr->rem_size < delta) {
        ret = qn_io_advance(srdr->src_rdr, srdr->rem_size);
        if (ret) srdr->rem_size = 0;
    } else {
        ret = qn_io_advance(srdr->src_rdr, delta);
        if (ret) srdr->rem_size -= delta;
    } // if
    return ret;
}

static qn_string qn_io_srdr_name_vfn(qn_io_reader_itf restrict itf)
{
    return qn_io_name((qn_io_srdr_from_io_reader(itf))->src_rdr);
}

static qn_fsize qn_io_srdr_size_vfn(qn_io_reader_itf restrict itf)
{
    return qn_io_srdr_from_io_reader(itf)->sec_size;
}

static qn_io_reader_st qn_io_srdr_vtable = {
    &qn_io_srdr_close_vfn,
    &qn_io_srdr_peek_vfn,
    &qn_io_srdr_read_vfn,
    &qn_io_srdr_seek_vfn,
    &qn_io_srdr_advance_vfn,
    NULL, // DUPLICATE
    NULL, // SECTION
    &qn_io_srdr_name_vfn,
    &qn_io_srdr_size_vfn
};

QN_SDK qn_io_section_reader_ptr qn_io_srdr_create(qn_io_reader_itf restrict src_rdr, size_t section_size)
{
    qn_io_section_reader_ptr new_srdr;

    new_srdr = calloc(1, sizeof(qn_io_section_reader_st));
    if (! new_srdr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_srdr->src_rdr = src_rdr;
    new_srdr->rem_size = section_size;
    new_srdr->sec_size = section_size;
    new_srdr->rdr_vtbl = &qn_io_srdr_vtable;
    return new_srdr;
}

QN_SDK void qn_io_srdr_destroy(qn_io_section_reader_ptr restrict srdr)
{
    if (srdr) {
        free(srdr);
    } // if
}

QN_SDK void qn_io_srdr_reset(qn_io_section_reader_ptr restrict srdr, qn_io_reader_itf restrict src_rdr, size_t section_size)
{
    assert(srdr);
    assert(src_rdr);
    assert(0 < section_size);

    srdr->src_rdr = src_rdr;
    srdr->rem_size = section_size;
    srdr->sec_size = section_size;
}

QN_SDK qn_io_reader_itf qn_io_srdr_to_io_reader(qn_io_section_reader_ptr restrict srdr)
{
    assert(srdr);
    return &srdr->rdr_vtbl;
}

#ifdef __cplusplus
}
#endif

