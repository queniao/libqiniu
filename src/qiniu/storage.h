#ifndef __QN_STORAGE_H__
#define __QN_STORAGE_H__

#include "qiniu/base/io.h"
#include "qiniu/base/json.h"
#include "qiniu/auth.h"
#include "qiniu/http.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Storage ----

struct _QN_STORAGE;
typedef struct _QN_STORAGE * qn_storage_ptr;

extern qn_storage_ptr qn_stor_create(void);
extern void qn_stor_destroy(qn_storage_ptr stor);

extern qn_json_object_ptr qn_stor_get_object_body(qn_storage_ptr stor);
extern qn_http_hdr_iterator_ptr qn_stor_resp_get_header_iterator(qn_storage_ptr stor);

// ---- Declaration of Management ----

typedef struct _QN_STOR_QUERY_EXTRA
{
    qn_mac_ptr mac;
    const qn_string acctoken;
} qn_stor_query_extra, *qn_stor_query_extra_ptr;

extern qn_bool qn_stor_stat(qn_storage_ptr stor, const char * restrict bucket, const char * restrict key, const qn_stor_query_extra_ptr restrict ext);

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
    qn_stor_put_method method;

    struct {
        const char * uptoken;
    } client_end;

    struct {
        qn_mac_ptr mac;
        qn_json_object_ptr put_policy;
    } server_end;

    const char * key;
    const char * crc32;
    const char * accept_type;
} qn_stor_put_extra, *qn_stor_put_extra_ptr;

extern qn_bool qn_stor_put_file(qn_storage_ptr stor, const char * fname, qn_stor_put_extra_ptr ext);
extern qn_bool qn_stor_put_buffer(qn_storage_ptr stor, const char * buf, int buf_size, qn_stor_put_extra_ptr ext);

// ----

struct _QN_STOR_RESUMABLE_PUT_SESSION;
typedef struct _QN_STOR_RESUMABLE_PUT_SESSION * qn_stor_rput_session_ptr;

extern qn_stor_rput_session_ptr qn_stor_rs_create(qn_fsize fsize);
extern void qn_stor_rs_destroy(qn_stor_rput_session_ptr ss);

extern qn_stor_rput_session_ptr qn_stor_rs_from_string(const char * str, int str_size);
extern qn_string qn_stor_rs_to_string(qn_stor_rput_session_ptr ss);

extern int qn_stor_rs_block_count(qn_stor_rput_session_ptr ss);
extern int qn_stor_rs_block_size(qn_stor_rput_session_ptr ss, int n);
extern qn_json_object_ptr qn_stor_rs_block_put_result(qn_stor_rput_session_ptr ss, int n);
extern qn_bool qn_stor_rs_is_putting_block_done(qn_stor_rput_session_ptr ss, int n);

typedef struct _QN_STOR_RESUMABLE_PUT_EXTRA
{
    struct {
        const char * uptoken;
    } client_end;

    struct {
        qn_mac_ptr mac;
        qn_json_object_ptr put_policy;
    } server_end;

    int chk_size;
    const char * final_key;
} qn_stor_rput_extra, *qn_stor_rput_extra_ptr;

extern qn_bool qn_stor_rp_put_chunk(qn_storage_ptr stor, qn_json_object_ptr blk_info, qn_io_reader_ptr rdr, int chk_size, qn_stor_rput_extra_ptr ext);
extern qn_bool qn_stor_rp_put_block(qn_storage_ptr stor, qn_json_object_ptr blk_info, qn_io_reader_ptr rdr, qn_stor_rput_extra_ptr ext);
extern qn_bool qn_stor_rp_make_file(qn_storage_ptr stor, qn_stor_rput_session_ptr ss, qn_stor_rput_extra_ptr ext);

extern qn_bool qn_stor_rp_put_file(qn_storage_ptr stor, qn_stor_rput_session_ptr * ss, const char * fname, qn_stor_rput_extra_ptr ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_STORAGE_H__

