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
    qn_rdr_pos pre_end;
    qn_rdr_pos post_end;
    qn_rdr_pos cap;
    qn_io_reader_ptr src_rdr;
    qn_rdr_entry entries[1];
} qn_reader;

QN_API qn_reader_ptr qn_rdr_create(qn_io_reader_ptr src_rdr, qn_rdr_pos filter_num)
{
    qn_reader_ptr new_rdr;

    if (filter_num < 1) filter_num = 1;

    new_rdr = calloc(1, sizeof(qn_reader) + sizeof(qn_rdr_entry) * (filter_num - 1));
    if (!new_rdr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_rdr->src_rdr = src_rdr;
    new_rdr->pre_end = 0;
    new_rdr->post_end = filter_num - 1;
    new_rdr->cap = filter_num;
    return new_rdr;
}

QN_API void qn_rdr_destroy(qn_reader_ptr restrict rdr)
{
    if (rdr) {
        free(rdr);
    } // if
}

QN_API void qn_rdr_reset(qn_reader_ptr restrict rdr)
{
    rdr->pre_end = 0;
    rdr->post_end = rdr->cap - 1;
}

QN_API qn_bool qn_rdr_add_pre_filter(qn_reader_ptr restrict rdr, void * restrict filter_data, qn_rdr_filter_callback filter_cb)
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

QN_API qn_bool qn_rdr_add_post_filter(qn_reader_ptr restrict rdr, void * restrict filter_data, qn_rdr_filter_callback filter_cb)
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

QN_API ssize_t qn_rdr_read(qn_reader_ptr restrict rdr, char * restrict buf, size_t size)
{
    char * real_buf = buf;
    size_t real_size = size;
    size_t reserved_size;
    ssize_t ret;
    qn_rdr_pos i;

    for (i = 0; i < rdr->pre_end; i += 1) {
        ret = rdr->entries[i].filter_cb(rdr->entries[i].filter_data, &real_buf, &real_size);
        switch (ret) {
            case QN_RDR_READING_ABORTED: qn_err_stor_set_putting_aborted_by_filter_pre_callback(); return ret;
            case QN_RDR_READING_FAILED: qn_err_fl_set_reading_file_failed(); return ret;
            default: break;
        } // switch
        if (ret != QN_RDR_SUCCEED) return ret;
    } // for

    reserved_size = buf - real_buf;
    ret = rdr->src_rdr->read(rdr->src_rdr->user_data, real_buf, real_size);
    if (ret < 0) return QN_RDR_READING_FAILED;

    real_buf = buf;
    real_size = reserved_size + ret;

    for (i = rdr->cap - 1; i > rdr->post_end; i -= 1) {
        ret = rdr->entries[i].filter_cb(rdr->entries[i].filter_data, &real_buf, &real_size);
        switch (ret) {
            case QN_RDR_READING_ABORTED: qn_err_stor_set_putting_aborted_by_filter_post_callback(); return ret;
            case QN_RDR_READING_FAILED: qn_err_fl_set_reading_file_failed(); return ret;
            default: break;
        } // switch
        if (ret != QN_RDR_SUCCEED) return ret;
    } // for

    if (buf < real_buf) memmove(buf, real_buf, real_size);

    return real_size;
}

#ifdef __cplusplus
}
#endif

