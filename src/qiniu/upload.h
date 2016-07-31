#ifndef __QN_UPLOAD_H__
#define __QN_UPLOAD_H__

#include "qiniu/base/string.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct _QN_UPLOADER;
typedef struct _QN_UPLOADER * qn_uploader_ptr;

extern qn_uploader_ptr qn_stor_up_create(void);
extern void qn_stor_up_destroy(qn_uploader_ptr up);

typedef enum _QN_UP_PUT_METHOD
{
    QN_UP_PUT_RESUMABLE = 0x1,
    QN_UP_PUT_BASE64 = 0x2,
    QN_UP_PUT_CHUNKED = 0x4
} qn_up_put_method;

struct _QN_UP_PUT_EXTRA
{
    const qn_string uptoken;
    qn_up_put_method method;
} qn_up_put_extra, *qn_up_put_extra_ptr;

extern qn_bool qn_stor_put_file(qn_uploader_ptr up, const char * local_file, qn_up_put_extra_ptr ext);
extern qn_bool qn_stor_put_buffer(qn_uploader_ptr up, const char * buf, qn_size buf_size, qn_up_put_extra_ptr ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_UPLOAD_H__

