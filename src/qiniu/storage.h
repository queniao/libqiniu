#ifndef __QN_STORAGE_H__
#define __QN_STORAGE_H__

#include "qiniu/base/json.h"
#include "qiniu/auth.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct _QN_STORAGE;
typedef struct _QN_STORAGE * qn_storage_ptr;

extern qn_storage_ptr qn_stor_mn_create(void);
extern void qn_stor_mn_destroy(qn_storage_ptr stor);

extern qn_json_object_ptr qn_stor_get_object_body(qn_storage_ptr stor);

typedef struct _QN_STOR_QUERY_EXTRA
{
    qn_mac_ptr mac;
    const qn_string acctoken;
} qn_stor_query_extra, *qn_stor_query_extra_ptr;

extern qn_bool qn_stor_mn_stat(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, const qn_stor_query_extra_ptr ext);

extern qn_bool qn_stor_mn_copy(qn_storage_ptr stor, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, const qn_stor_query_extra_ptr ext);
extern qn_bool qn_stor_mn_move(qn_storage_ptr stor, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, const qn_stor_query_extra_ptr ext);
extern qn_bool qn_stor_mn_delete(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, const qn_stor_query_extra_ptr ext);
extern qn_bool qn_stor_mn_change_mime(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, const char * restrict mime, const qn_stor_query_extra_ptr ext);

typedef struct _QN_STOR_FETCH_EXTRA
{
} qn_stor_fetch_extra, *qn_stor_fetch_extra_ptr;

extern qn_bool qn_stor_mn_fetch(qn_storage_ptr stor, const char * restrict url, const char * restrict bucket, const char * restrict key, qn_stor_fetch_extra_ptr ext);
extern qn_bool qn_stor_mn_prefetch(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, qn_stor_fetch_extra_ptr ext);

typedef struct _QN_STOR_LIST_EXTRA
{
    const char * prefix;
    const char * delimiter;
    int limit;
} qn_stor_list_extra, *qn_stor_list_extra_ptr;

extern qn_bool qn_stor_mn_list(qn_storage_ptr stor, const char * restrict bucket, qn_stor_list_extra_ptr ext);

struct _QN_STOR_BATCH;
typedef struct _QN_STOR_BATCH * qn_stor_batch_ptr;

extern qn_stor_batch_ptr qn_stor_mn_create_batch(void);
extern void qn_stor_mn_destroy_batch(qn_stor_batch_ptr bt);
extern void qn_stor_mn_reset_batch(qn_stor_batch_ptr bt);

extern qn_bool qn_stor_mn_batch_operate(qn_storage_ptr stor, qn_stor_batch_ptr bt);

#ifdef __cplusplus
}
#endif

#endif // __QN_STORAGE_H__

