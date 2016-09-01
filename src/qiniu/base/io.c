#include "qiniu/base/io.h"

#ifdef __cplusplus
extern "C"
{
#endif

int qn_io_srd_read(void * user_data, char * buf, int buf_size)
{
    qn_io_section_reader_ptr srdr = (qn_io_section_reader_ptr) user_data;
    int reading_bytes;
    int ret;

    reading_bytes = srdr->size - srdr->pos;
    if (reading_bytes > buf_size) reading_bytes = buf_size;

    ret = srdr->rdr->read(srdr->rdr->user_data, buf, reading_bytes);
    if (ret < 0) return -1;

    srdr->pos += ret;
    return ret;
}

void qn_io_srd_init(qn_io_section_reader_ptr srdr, qn_io_reader_ptr rdr, int size)
{
    srdr->rdr = rdr;
    srdr->size = size;
    srdr->pos = 0;
}

#ifdef __cplusplus
}
#endif
