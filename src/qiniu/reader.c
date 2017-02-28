#include <string.h>
#include <stdlib.h>

#include "qiniu/base/errors.h"
#include "qiniu/reader.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _QN_RDR_ENTRY
{
    void * filter_data;
    qn_rdr_filter_callback filter_cb;
} qn_rdr_entry, *qn_rdr_entry_ptr;

typedef struct _QN_READER
{
    qn_io_reader_ptr rdr_vtbl;
    qn_io_reader_itf src_rdr;
    qn_rdr_pos pre_end;
    qn_rdr_pos post_end;
    qn_rdr_pos cap;
    qn_rdr_pos auto_close_source:1;
    qn_rdr_entry entries[1];
} qn_reader_st;

static inline qn_reader_ptr qn_rdr_from_io_reader(qn_io_reader_itf restrict itf)
{
    return (qn_reader_ptr)( ( (char *) itf ) - (char *)( &((qn_reader_ptr)0)->rdr_vtbl ) );
}

static void qn_rdr_close_fn(qn_io_reader_itf restrict itf)
{
    qn_rdr_destroy(qn_rdr_from_io_reader(itf));
}

static ssize_t qn_rdr_peek_fn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_rdr_peek(qn_rdr_from_io_reader(itf), buf, buf_size);
}

static ssize_t qn_rdr_read_fn(qn_io_reader_itf restrict itf, char * restrict buf, size_t buf_size)
{
    return qn_rdr_read(qn_rdr_from_io_reader(itf), buf, buf_size);
}

static qn_bool qn_rdr_seek_fn(qn_io_reader_itf restrict itf, qn_fsize offset)
{
    return qn_rdr_seek(qn_rdr_from_io_reader(itf), offset);
}

static qn_bool qn_rdr_advance_fn(qn_io_reader_itf restrict itf, size_t delta)
{
    return qn_rdr_advance(qn_rdr_from_io_reader(itf), delta);
}

static qn_io_reader_itf qn_rdr_duplicate_fn(qn_io_reader_itf restrict itf)
{
    qn_reader_ptr new_rdr = qn_rdr_duplicate(qn_rdr_from_io_reader(itf));
    if (!new_rdr) return NULL;
    return qn_rdr_to_io_reader(new_rdr);
}

static qn_io_reader_itf qn_rdr_section_fn(qn_io_reader_itf restrict itf, qn_fsize offset, size_t sec_size)
{
    qn_reader_ptr new_rdr = qn_rdr_section(qn_rdr_from_io_reader(itf), offset, sec_size);
    if (!new_rdr) return NULL;
    return qn_rdr_to_io_reader(new_rdr);
}

static qn_string qn_rdr_name_fn(qn_io_reader_itf restrict itf)
{
    return qn_rdr_name(qn_rdr_from_io_reader(itf));
}

static qn_fsize qn_rdr_size_fn(qn_io_reader_itf restrict itf)
{
    return qn_rdr_size(qn_rdr_from_io_reader(itf));
}

static qn_io_reader_st qn_rdr_vtable = {
    &qn_rdr_close_fn,
    &qn_rdr_peek_fn,
    &qn_rdr_read_fn,
    &qn_rdr_seek_fn,
    &qn_rdr_advance_fn,
    &qn_rdr_duplicate_fn,
    &qn_rdr_section_fn,
    &qn_rdr_name_fn,
    &qn_rdr_size_fn
};

QN_SDK qn_reader_ptr qn_rdr_create(qn_io_reader_itf src_rdr, qn_rdr_pos filter_num)
{
    qn_reader_ptr new_rdr;

    if (filter_num < 1) filter_num = 1;

    new_rdr = calloc(1, sizeof(qn_reader_st) + sizeof(qn_rdr_entry) * (filter_num - 1));
    if (!new_rdr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_rdr->src_rdr = qn_io_duplicate(src_rdr);
    if (!new_rdr->src_rdr) {
        free(new_rdr);
        return NULL;
    } // if

    new_rdr->pre_end = 0;
    new_rdr->post_end = filter_num - 1;
    new_rdr->cap = filter_num;

    new_rdr->rdr_vtbl = &qn_rdr_vtable;
    return new_rdr;
}

QN_SDK void qn_rdr_destroy(qn_reader_ptr restrict rdr)
{
    if (rdr) {
        if (rdr->auto_close_source) qn_io_close(rdr->src_rdr);
        free(rdr);
    } // if
}

QN_SDK void qn_rdr_reset(qn_reader_ptr restrict rdr)
{
    rdr->pre_end = 0;
    rdr->post_end = rdr->cap - 1;
}

QN_SDK qn_io_reader_itf qn_rdr_to_io_reader(qn_reader_ptr restrict rdr)
{
    return &rdr->rdr_vtbl;
}

QN_SDK qn_bool qn_rdr_add_pre_filter(qn_reader_ptr restrict rdr, void * restrict filter_data, qn_rdr_filter_callback filter_cb)
{
    if (rdr->post_end < rdr->pre_end) {
        qn_err_set_out_of_capacity();
        return qn_false;
    } // if

    rdr->entries[rdr->pre_end].filter_data = filter_data;
    rdr->entries[rdr->pre_end].filter_cb = filter_cb;
    rdr->pre_end += 1;
    return qn_true;
}

QN_SDK qn_bool qn_rdr_add_post_filter(qn_reader_ptr restrict rdr, void * restrict filter_data, qn_rdr_filter_callback filter_cb)
{
    if (rdr->post_end < rdr->pre_end) {
        qn_err_set_out_of_capacity();
        return qn_false;
    } // if

    rdr->entries[rdr->post_end].filter_data = filter_data;
    rdr->entries[rdr->post_end].filter_cb = filter_cb;
    rdr->post_end -= 1;
    return qn_true;
}

static ssize_t qn_rdr_do_read(qn_reader_ptr restrict rdr, char * restrict buf, size_t size, qn_bool only_peek)
{
    char * real_buf = buf;
    size_t real_size = size;
    size_t reserved_size;
    ssize_t ret;
    qn_rdr_pos i;

    for (i = 0; i < rdr->pre_end; i += 1) {
        ret = rdr->entries[i].filter_cb(rdr->entries[i].filter_data, &real_buf, &real_size);
        switch (ret) {
            case QN_IO_RDR_READING_ABORTED: qn_err_stor_set_upload_aborted_by_filter_pre_callback(); return ret;
            case QN_IO_RDR_READING_FAILED: qn_err_fl_set_reading_file_failed(); return ret;
            default: if (ret <= 0) return ret;
        } // switch
    } // for

    reserved_size = buf - real_buf;
    if (only_peek) {
        ret = qn_io_peek(rdr->src_rdr, real_buf, real_size);
    } else {
        ret = qn_io_read(rdr->src_rdr, real_buf, real_size);
    } // if
    if (ret < 0) return ret;

    real_buf = buf;
    real_size = reserved_size + ret;

    for (i = rdr->cap - 1; i > rdr->post_end; i -= 1) {
        ret = rdr->entries[i].filter_cb(rdr->entries[i].filter_data, &real_buf, &real_size);
        switch (ret) {
            case QN_IO_RDR_READING_ABORTED: qn_err_stor_set_upload_aborted_by_filter_post_callback(); return ret;
            case QN_IO_RDR_READING_FAILED: qn_err_fl_set_reading_file_failed(); return ret;
            default: if (ret <= 0) return ret;
        } // switch
    } // for

    if (buf < real_buf) memmove(buf, real_buf, real_size);
    return real_size;
}

QN_SDK ssize_t qn_rdr_peek(qn_reader_ptr restrict rdr, char * restrict buf, size_t size)
{
    return qn_rdr_do_read(rdr, buf, size, qn_true);
}

QN_SDK ssize_t qn_rdr_read(qn_reader_ptr restrict rdr, char * restrict buf, size_t size)
{
    return qn_rdr_do_read(rdr, buf, size, qn_false);
}

QN_SDK qn_bool qn_rdr_seek(qn_reader_ptr restrict rdr, qn_fsize offset)
{
    return qn_io_seek(rdr->src_rdr, offset);
}

QN_SDK qn_bool qn_rdr_advance(qn_reader_ptr restrict rdr, size_t delta)
{
    return qn_io_advance(rdr->src_rdr, delta);
}

static qn_reader_ptr qn_rdr_do_duplicate(qn_reader_ptr restrict rdr, qn_io_reader_itf restrict src_rdr)
{
    size_t size = sizeof(qn_reader_st) + sizeof(qn_rdr_entry) * (rdr->cap - 1);
    qn_reader_ptr new_rdr = malloc(size);
    if (!new_rdr) {
        qn_err_set_out_of_memory();
        qn_io_close(src_rdr);
        return NULL;
    } // if

    memcpy(new_rdr, rdr, size);
    new_rdr->src_rdr = src_rdr;
    new_rdr->auto_close_source = 1;
    return new_rdr;
}

QN_SDK qn_reader_ptr qn_rdr_duplicate(qn_reader_ptr restrict rdr)
{
    qn_io_reader_itf new_src_rdr = qn_io_duplicate(rdr->src_rdr);
    if (!new_src_rdr) return NULL;
    return qn_rdr_do_duplicate(rdr, new_src_rdr);
}

QN_SDK qn_reader_ptr qn_rdr_section(qn_reader_ptr restrict rdr, qn_fsize offset, size_t sec_size)
{
    qn_io_reader_itf new_src_rdr = qn_io_section(rdr->src_rdr, offset, sec_size);
    if (!new_src_rdr) return NULL;
    return qn_rdr_do_duplicate(rdr, new_src_rdr);
}

QN_SDK qn_string qn_rdr_name(qn_reader_ptr restrict rdr)
{
    return qn_io_name(rdr->src_rdr);
}

QN_SDK qn_fsize qn_rdr_size(qn_reader_ptr restrict rdr)
{
    return qn_io_size(rdr->src_rdr);
}

#ifdef __cplusplus
}
#endif

