#include "qiniu/base/io.h"

#ifdef __cplusplus
extern "C"
{
#endif

QN_API ssize_t qn_io_srd_read(void * restrict user_data, char * restrict buf, size_t buf_size)
{
    qn_io_section_reader_ptr srdr = (qn_io_section_reader_ptr) user_data;
    size_t reading_bytes;
    int ret;

    reading_bytes = srdr->size - srdr->pos;
    if (reading_bytes > buf_size) reading_bytes = buf_size;

    ret = srdr->rdr->read(srdr->rdr->user_data, buf, reading_bytes);
    if (ret < 0) return -1;

    srdr->pos += ret;
    return ret;
}

QN_API void qn_io_srd_init(qn_io_section_reader_ptr restrict srdr, qn_io_reader_ptr restrict rdr, size_t size)
{
    srdr->rdr = rdr;
    srdr->size = size;
    srdr->pos = 0;
}

#ifdef __cplusplus
}
#endif
