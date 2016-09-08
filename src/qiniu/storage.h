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
extern qn_json_array_ptr qn_stor_get_array_body(qn_storage_ptr stor);
extern qn_http_hdr_iterator_ptr qn_stor_resp_get_header_iterator(qn_storage_ptr stor);

// ---- Declaration of Management ----

typedef struct _QN_STOR_AUTH
{
    struct {
        qn_mac_ptr mac;
    } server_end;

    struct {
        union {
            const qn_string acctoken;
            const qn_string uptoken;
        };
    } client_end;
} qn_stor_auth, *qn_stor_auth_ptr;

typedef struct _QN_STOR_STAT_EXTRA
{
} qn_stor_stat_extra, *qn_stor_stat_extra_ptr;

extern qn_bool qn_stor_stat(qn_storage_ptr stor, const qn_stor_auth_ptr restrict auth, const char * restrict bucket, const char * restrict key, qn_stor_stat_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_COPY_EXTRA
{
    qn_bool force;
} qn_stor_copy_extra, *qn_stor_copy_extra_ptr;

extern qn_bool qn_stor_copy(qn_storage_ptr stor, const qn_stor_auth_ptr restrict auth, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_copy_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_MOVE_EXTRA
{
} qn_stor_move_extra, *qn_stor_move_extra_ptr;

extern qn_bool qn_stor_move(qn_storage_ptr stor, const qn_stor_auth_ptr restrict auth, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_move_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_DELETE_EXTRA
{
} qn_stor_delete_extra, *qn_stor_delete_extra_ptr;

extern qn_bool qn_stor_delete(qn_storage_ptr stor, const qn_stor_auth_ptr restrict auth, const char * restrict bucket, const char * restrict key, qn_stor_delete_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_CHANGE_MIME_EXTRA
{
} qn_stor_change_mime_extra, *qn_stor_change_mime_extra_ptr;

extern qn_bool qn_stor_change_mime(qn_storage_ptr stor, const qn_stor_auth_ptr restrict auth, const char * restrict bucket, const char * restrict key, const char * restrict mime, qn_stor_change_mime_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_FETCH_EXTRA
{
} qn_stor_fetch_extra, *qn_stor_fetch_extra_ptr;


extern qn_bool qn_stor_fetch(qn_storage_ptr stor, const qn_stor_auth_ptr restrict auth, const char * restrict src_url, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr ext);
extern qn_bool qn_stor_prefetch(qn_storage_ptr stor, const qn_stor_auth_ptr restrict auth, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr ext);

// ----

typedef qn_bool (*qn_stor_item_processor_callback)(void * user_data, qn_json_object_ptr item);

typedef struct _QN_STOR_LIST_EXTRA
{
    const char * prefix;
    const char * delimiter;
    int limit;

    void * item_processor;
    qn_stor_item_processor_callback item_processor_callback;
} qn_stor_list_extra, *qn_stor_list_extra_ptr;

extern qn_bool qn_stor_list(qn_storage_ptr stor, const qn_stor_auth_ptr restrict auth, const char * restrict bucket, qn_stor_list_extra_ptr restrict ext);

// ----

struct _QN_STOR_BATCH;
typedef struct _QN_STOR_BATCH * qn_stor_batch_ptr;

extern qn_stor_batch_ptr qn_stor_bt_create(void);
extern void qn_stor_bt_destroy(qn_stor_batch_ptr bt);
extern void qn_stor_bt_reset(qn_stor_batch_ptr bt);

extern qn_bool qn_stor_bt_add_stat_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key);
extern qn_bool qn_stor_bt_add_copy_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key);
extern qn_bool qn_stor_bt_add_move_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key);
extern qn_bool qn_stor_bt_add_delete_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key);

extern qn_bool qn_stor_execute_batch_opertions(qn_storage_ptr restrict stor, const qn_stor_auth_ptr restrict auth, const qn_stor_batch_ptr restrict bt);

// ---- Declaration of Upload ----

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

    const char * final_key;
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

