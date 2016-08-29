#ifndef __QN_STORAGE_H__
#define __QN_STORAGE_H__

#include "qiniu/base/json.h"
#include "qiniu/auth.h"
#include "qiniu/http.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct _QN_STORAGE;
typedef struct _QN_STORAGE * qn_storage_ptr;

extern qn_storage_ptr qn_stor_create(void);
extern void qn_stor_destroy(qn_storage_ptr stor);

extern qn_json_object_ptr qn_stor_get_object_body(qn_storage_ptr stor);
extern qn_http_hdr_iterator_ptr qn_stor_resp_get_header_iterator(qn_storage_ptr stor);

typedef struct _QN_STOR_QUERY_EXTRA
{
    qn_mac_ptr mac;
    const qn_string acctoken;
} qn_stor_query_extra, *qn_stor_query_extra_ptr;

extern qn_bool qn_stor_stat(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, const qn_stor_query_extra_ptr ext);

extern qn_bool qn_stor_copy(qn_storage_ptr stor, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, const qn_stor_query_extra_ptr ext);
extern qn_bool qn_stor_move(qn_storage_ptr stor, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, const qn_stor_query_extra_ptr ext);
extern qn_bool qn_stor_delete(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, const qn_stor_query_extra_ptr ext);
extern qn_bool qn_stor_change_mime(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, const char * restrict mime, const qn_stor_query_extra_ptr ext);

typedef struct _QN_STOR_FETCH_EXTRA
{
} qn_stor_fetch_extra, *qn_stor_fetch_extra_ptr;

extern qn_bool qn_stor_fetch(qn_storage_ptr stor, const char * restrict url, const char * restrict bucket, const char * restrict key, qn_stor_fetch_extra_ptr ext);
extern qn_bool qn_stor_prefetch(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, qn_stor_fetch_extra_ptr ext);

typedef struct _QN_STOR_LIST_EXTRA
{
    const char * prefix;
    const char * delimiter;
    int limit;
} qn_stor_list_extra, *qn_stor_list_extra_ptr;

extern qn_bool qn_stor_list(qn_storage_ptr stor, const char * restrict bucket, qn_stor_list_extra_ptr ext);

struct _QN_STOR_BATCH;
typedef struct _QN_STOR_BATCH * qn_stor_batch_ptr;

extern qn_stor_batch_ptr qn_stor_create_batch(void);
extern void qn_stor_destroy_batch(qn_stor_batch_ptr bt);
extern void qn_stor_reset_batch(qn_stor_batch_ptr bt);

extern qn_bool qn_stor_batch_operate(qn_storage_ptr stor, qn_stor_batch_ptr bt);

// ---- Declaration of upload functions

typedef enum _QN_STOR_PUT_METHOD
{
    QN_STOR_PUT_RESUMABLE = 0x1,
    QN_STOR_PUT_BASE64 = 0x2,
    QN_STOR_PUT_CHUNKED = 0x4
} qn_stor_put_method;

typedef struct _QN_STOR_PUT_EXTRA
{
    const qn_string uptoken;
    qn_stor_put_method method;
} qn_stor_put_extra, *qn_stor_put_extra_ptr;

extern qn_bool qn_stor_put_file(qn_storage_ptr stor, const char * local_file, qn_stor_put_extra_ptr ext);
extern qn_bool qn_stor_put_buffer(qn_storage_ptr stor, const char * buf, qn_size buf_size, qn_stor_put_extra_ptr ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_STORAGE_H__

