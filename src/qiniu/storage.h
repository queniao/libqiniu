/***************************************************************************//**
* @file qiniu/storage.h
* @brief This header file declares all core functions of Qiniu Cloud Storage.
*
* AUTHOR      : liangtao@qiniu.com (QQ: 510857)
* COPYRIGHT   : 2016(c) Shanghai Qiniu Information Technologies Co., Ltd.
* DESCRIPTION :
*
* This header file declares all core functions of Qiniu Cloud Storage, that 
* are used to upload and manage files.
*******************************************************************************/

#ifndef __QN_STORAGE_H__
#define __QN_STORAGE_H__

#include "qiniu/base/io.h"
#include "qiniu/base/json.h"
#include "qiniu/auth.h"
#include "qiniu/http.h"
#include "qiniu/region.h"
#include "qiniu/reader.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Storage Extension ----

/***************************************************************************//**
* @defgroup Storage-Extension Storage Extension Interface
*
*******************************************************************************/

typedef struct _QN_STOR_RGN
{
    qn_rgn_table_ptr rtbl;
    qn_rgn_entry_ptr entry;
} qn_stor_rgn, *qn_stor_rgn_ptr;

typedef struct _QN_STOR_PUT_CTRL
{
    qn_fsize fsize;
    qn_io_reader_itf rdr;
} qn_stor_put_ctrl, *qn_stor_put_ctrl_ptr;

// ---- Declaration of Storage Basic Functions ----

/***************************************************************************//**
* @defgroup Storage-Basic Storage Basic Functions
*
*******************************************************************************/

struct _QN_STORAGE;
typedef struct _QN_STORAGE * qn_storage_ptr;

QN_API extern qn_storage_ptr qn_stor_create(void);
QN_API extern void qn_stor_destroy(qn_storage_ptr restrict stor);

QN_API extern qn_json_object_ptr qn_stor_get_object_body(const qn_storage_ptr restrict stor);
QN_API extern qn_json_array_ptr qn_stor_get_array_body(const qn_storage_ptr restrict stor);
QN_API extern qn_http_hdr_iterator_ptr qn_stor_resp_get_header_iterator(const qn_storage_ptr restrict stor);

// ---- Declaration of Storage Management Functions ----

/***************************************************************************//**
* @defgroup Storage-Management Storage Managment Functions
*
*******************************************************************************/

// ----

typedef struct _QN_STOR_STAT_EXTRA
{
    // ---- Extensions ----
    // Multi-Region : Pass the host entry information of a storage region.
    qn_stor_rgn rgn;
} qn_stor_stat_extra, *qn_stor_stat_extra_ptr;

QN_API extern qn_json_object_ptr qn_stor_stat(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_stat_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_COPY_EXTRA
{
    qn_bool force;

    // ---- Extensions ----
    // Multi-Region : Pass the host entry information of a storage region.
    qn_stor_rgn rgn;
} qn_stor_copy_extra, *qn_stor_copy_extra_ptr;

QN_API extern qn_json_object_ptr qn_stor_copy(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_copy_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_MOVE_EXTRA
{
    // ---- Extensions ----
    // Multi-Region : Pass the host entry information of a storage region.
    qn_stor_rgn rgn;
} qn_stor_move_extra, *qn_stor_move_extra_ptr;

QN_API extern qn_json_object_ptr qn_stor_move(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_move_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_DELETE_EXTRA
{
    // ---- Extensions ----
    // Multi-Region : Pass the host entry information of a storage region.
    qn_stor_rgn rgn;
} qn_stor_delete_extra, *qn_stor_delete_extra_ptr;

QN_API extern qn_json_object_ptr qn_stor_delete(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_delete_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_CHANGE_MIME_EXTRA
{
    // ---- Extensions ----
    // Multi-Region : Pass the host entry information of a storage region.
    qn_stor_rgn rgn;
} qn_stor_change_mime_extra, *qn_stor_change_mime_extra_ptr;

QN_API extern qn_json_object_ptr qn_stor_change_mime(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, const char * restrict mime, qn_stor_change_mime_extra_ptr restrict ext);

// ----

typedef struct _QN_STOR_FETCH_EXTRA
{
    // ---- Extensions ----
    // Multi-Region : Pass the host entry information of a storage region.
    qn_stor_rgn rgn;
} qn_stor_fetch_extra, *qn_stor_fetch_extra_ptr;

QN_API extern qn_json_object_ptr qn_stor_fetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_url, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict ext);
QN_API extern qn_json_object_ptr qn_stor_prefetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict ext);

// ----

typedef qn_bool (*qn_stor_item_processor_callback)(void * user_data, qn_json_object_ptr item);

typedef struct _QN_STOR_LIST_EXTRA
{
    const char * prefix;
    const char * delimiter;
    int limit;

    void * item_processor;
    qn_stor_item_processor_callback item_processor_callback;

    // ---- Extensions ----
    // Multi-Region : Pass the host entry information of a storage region.
    qn_stor_rgn rgn;
} qn_stor_list_extra, *qn_stor_list_extra_ptr;

QN_API extern qn_json_object_ptr qn_stor_list(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, qn_stor_list_extra_ptr restrict ext);

// ----

struct _QN_STOR_BATCH;
typedef struct _QN_STOR_BATCH * qn_stor_batch_ptr;

typedef struct _QN_STOR_BATCH_EXTRA
{
    // ---- Extensions ----
    // Multi-Region : Pass the host entry information of a storage region.
    qn_stor_rgn rgn;
} qn_stor_batch_extra, *qn_stor_batch_extra_ptr;

QN_API extern qn_stor_batch_ptr qn_stor_bt_create(void);
QN_API extern void qn_stor_bt_destroy(qn_stor_batch_ptr restrict bt);
QN_API extern void qn_stor_bt_reset(qn_stor_batch_ptr restrict bt);

QN_API extern qn_bool qn_stor_bt_add_stat_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key);
QN_API extern qn_bool qn_stor_bt_add_copy_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key);
QN_API extern qn_bool qn_stor_bt_add_move_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key);
QN_API extern qn_bool qn_stor_bt_add_delete_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key);

QN_API extern qn_json_object_ptr qn_stor_execute_batch_opertions(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const qn_stor_batch_ptr restrict bt, qn_stor_batch_extra_ptr restrict ext);

// ---- Declaration of Upload ----

struct _QN_STOR_PUT_EXTRA;
typedef struct _QN_STOR_PUT_EXTRA * qn_stor_put_extra_ptr;

QN_API extern qn_stor_put_extra_ptr qn_stor_pe_create(void);
QN_API extern void qn_stor_pe_destroy(qn_stor_put_extra_ptr restrict pe);
QN_API extern void qn_stor_pe_reset(qn_stor_put_extra_ptr restrict pe);

QN_API extern void qn_stor_pe_set_final_key(qn_stor_put_extra_ptr restrict pe, const char * restrict final_key);
QN_API extern void qn_stor_pe_set_local_crc32(qn_stor_put_extra_ptr restrict pe, const char * restrict crc32);
QN_API extern void qn_stor_pe_set_accept_type(qn_stor_put_extra_ptr restrict pe, const char * restrict accept_type);
QN_API extern void qn_stor_pe_set_region_entry(qn_stor_put_extra_ptr restrict pe, qn_rgn_entry_ptr restrict entry);
QN_API extern void qn_stor_pe_set_source_reader(qn_stor_put_extra_ptr restrict pe, qn_io_reader_itf restrict rdr, qn_fsize fsize, qn_bool detect_fsize);

QN_API extern qn_json_object_ptr qn_stor_put_file(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict fname, qn_stor_put_extra_ptr restrict pe);
QN_API extern qn_json_object_ptr qn_stor_put_buffer(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict buf, int buf_size, qn_stor_put_extra_ptr restrict pe);

// ----

/***************************************************************************//**
* @defgroup Storage-Resumable-Put Resumable Put Functions
*******************************************************************************/

struct _QN_STOR_RESUMABLE_PUT_SESSION;
typedef struct _QN_STOR_RESUMABLE_PUT_SESSION * qn_stor_rput_session_ptr;

QN_API extern qn_stor_rput_session_ptr qn_stor_rs_create(qn_fsize fsize);
QN_API extern void qn_stor_rs_destroy(qn_stor_rput_session_ptr restrict ss);

QN_API extern qn_stor_rput_session_ptr qn_stor_rs_from_string(const char * restrict str, size_t str_size);
QN_API extern qn_string qn_stor_rs_to_string(const qn_stor_rput_session_ptr restrict ss);

QN_API extern int qn_stor_rs_block_count(const qn_stor_rput_session_ptr restrict ss);
QN_API extern int qn_stor_rs_block_size(const qn_stor_rput_session_ptr restrict ss, int n);
QN_API extern qn_json_object_ptr qn_stor_rs_block_info(const qn_stor_rput_session_ptr restrict ss, int n);
QN_API extern qn_bool qn_stor_rs_is_putting_block_done(const qn_stor_rput_session_ptr restrict ss, int n);

struct _QN_STOR_RESUMABLE_PUT_EXTRA;
typedef struct _QN_STOR_RESUMABLE_PUT_EXTRA * qn_stor_rput_extra_ptr;

QN_API extern qn_stor_rput_extra_ptr qn_stor_rpe_create(void);
QN_API extern void qn_stor_rpe_destroy(qn_stor_rput_extra_ptr restrict rpe);
QN_API extern void qn_stor_rpe_reset(qn_stor_rput_extra_ptr restrict rpe);

QN_API extern void qn_stor_rpe_set_chunk_size(qn_stor_rput_extra_ptr restrict rpe, int chk_size);
QN_API extern void qn_stor_rpe_set_final_key(qn_stor_rput_extra_ptr restrict rpe, const char * restrict key);
QN_API extern void qn_stor_rpe_set_region_entry(qn_stor_rput_extra_ptr restrict rpe, qn_rgn_entry_ptr restrict entry);

// ----

QN_API extern qn_json_object_ptr qn_stor_rp_put_chunk(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_json_object_ptr restrict blk_info, qn_io_reader_itf restrict rdr, int chk_size, qn_stor_rput_extra_ptr restrict ext);
QN_API extern qn_json_object_ptr qn_stor_rp_put_block(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_json_object_ptr restrict blk_info, qn_io_reader_itf restrict rdr, qn_stor_rput_extra_ptr restrict ext);
QN_API extern qn_json_object_ptr qn_stor_rp_make_file(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_rput_session_ptr restrict ss, qn_stor_rput_extra_ptr restrict ext);

QN_API extern qn_json_object_ptr qn_stor_rp_put_file(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_rput_session_ptr * restrict ss, const char * restrict fname, qn_stor_rput_extra_ptr restrict ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_STORAGE_H__

