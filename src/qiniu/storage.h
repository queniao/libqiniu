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

struct _QN_STOR_STAT_EXTRA;
typedef struct _QN_STOR_STAT_EXTRA * qn_stor_stat_extra_ptr;

QN_API extern qn_stor_stat_extra_ptr qn_stor_se_create(void);
QN_API extern void qn_stor_se_destroy(qn_stor_stat_extra_ptr restrict se);
QN_API extern void qn_stor_se_reset(qn_stor_stat_extra_ptr restrict se);

QN_API extern void qn_stor_se_set_region_entry(qn_stor_stat_extra_ptr restrict se, qn_rgn_entry_ptr restrict entry);

QN_API extern qn_json_object_ptr qn_stor_stat(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_stat_extra_ptr restrict ext);

// ----

struct _QN_STOR_COPY_EXTRA;
typedef struct _QN_STOR_COPY_EXTRA * qn_stor_copy_extra_ptr;

QN_API extern qn_stor_copy_extra_ptr qn_stor_ce_create(void);
QN_API extern void qn_stor_ce_destroy(qn_stor_copy_extra_ptr restrict ce);
QN_API extern void qn_stor_ce_reset(qn_stor_copy_extra_ptr restrict ce);

QN_API extern void qn_stor_ce_set_force_overwrite(qn_stor_copy_extra_ptr restrict ce, qn_bool force);
QN_API extern void qn_stor_ce_set_region_entry(qn_stor_copy_extra_ptr restrict ce, qn_rgn_entry_ptr restrict entry);

QN_API extern qn_json_object_ptr qn_stor_copy(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_copy_extra_ptr restrict ext);

// ----

struct _QN_STOR_MOVE_EXTRA;
typedef struct _QN_STOR_MOVE_EXTRA * qn_stor_move_extra_ptr;

QN_API extern qn_stor_move_extra_ptr qn_stor_me_create(void);
QN_API extern void qn_stor_me_destroy(qn_stor_move_extra_ptr restrict me);
QN_API extern void qn_stor_me_reset(qn_stor_move_extra_ptr restrict me);

QN_API extern void qn_stor_me_set_region_entry(qn_stor_move_extra_ptr restrict me, qn_rgn_entry_ptr restrict entry);

QN_API extern qn_json_object_ptr qn_stor_move(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_move_extra_ptr restrict ext);

// ----

struct _QN_STOR_DELETE_EXTRA;
typedef struct _QN_STOR_DELETE_EXTRA * qn_stor_delete_extra_ptr;

QN_API extern qn_stor_delete_extra_ptr qn_stor_de_create(void);
QN_API extern void qn_stor_de_destroy(qn_stor_delete_extra_ptr restrict de);
QN_API extern void qn_stor_de_reset(qn_stor_delete_extra_ptr restrict de);

QN_API extern void qn_stor_de_set_region_entry(qn_stor_delete_extra_ptr restrict de, qn_rgn_entry_ptr restrict entry);

QN_API extern qn_json_object_ptr qn_stor_delete(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_delete_extra_ptr restrict ext);

// ----

struct _QN_STOR_CHANGE_MIME_EXTRA;
typedef struct _QN_STOR_CHANGE_MIME_EXTRA * qn_stor_change_mime_extra_ptr;

QN_API extern qn_stor_change_mime_extra_ptr qn_stor_cme_create(void);
QN_API extern void qn_stor_cme_destroy(qn_stor_change_mime_extra_ptr restrict cme);
QN_API extern void qn_stor_cme_reset(qn_stor_change_mime_extra_ptr restrict cme);

QN_API extern void qn_stor_cme_set_region_entry(qn_stor_change_mime_extra_ptr restrict cme, qn_rgn_entry_ptr restrict entry);

QN_API extern qn_json_object_ptr qn_stor_change_mime(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, const char * restrict mime, qn_stor_change_mime_extra_ptr restrict ext);

// ----

struct _QN_STOR_FETCH_EXTRA;
typedef struct _QN_STOR_FETCH_EXTRA * qn_stor_fetch_extra_ptr;

QN_API extern qn_stor_fetch_extra_ptr qn_stor_fe_create(void);
QN_API extern void qn_stor_fe_destroy(qn_stor_fetch_extra_ptr restrict fe);
QN_API extern void qn_stor_fe_reset(qn_stor_fetch_extra_ptr restrict fe);

QN_API extern void qn_stor_fe_set_region_entry(qn_stor_fetch_extra_ptr restrict fe, qn_rgn_entry_ptr restrict entry);

QN_API extern qn_json_object_ptr qn_stor_fetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_url, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict ext);
QN_API extern qn_json_object_ptr qn_stor_prefetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict ext);

// ----

struct _QN_STOR_LIST_EXTRA;
typedef struct _QN_STOR_LIST_EXTRA * qn_stor_list_extra_ptr;

QN_API extern qn_stor_list_extra_ptr qn_stor_le_create(void);
QN_API extern void qn_stor_le_destroy(qn_stor_list_extra_ptr restrict le);
QN_API extern void qn_stor_le_reset(qn_stor_list_extra_ptr restrict le);

QN_API extern void qn_stor_le_set_prefix(qn_stor_list_extra_ptr restrict le, const char * restrict prefix);
QN_API extern void qn_stor_le_set_delimiter(qn_stor_list_extra_ptr restrict le, const char * restrict delimiter);
QN_API extern void qn_stor_le_set_marker(qn_stor_list_extra_ptr restrict le, const char * restrict marker);
QN_API extern void qn_stor_le_set_limit(qn_stor_list_extra_ptr restrict le, int limit);

QN_API extern qn_json_object_ptr qn_stor_list(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, qn_stor_list_extra_ptr restrict ext);

// ----

struct _QN_STOR_BATCH;
typedef struct _QN_STOR_BATCH * qn_stor_batch_ptr;

QN_API extern qn_stor_batch_ptr qn_stor_bt_create(void);
QN_API extern void qn_stor_bt_destroy(qn_stor_batch_ptr restrict bt);
QN_API extern void qn_stor_bt_reset(qn_stor_batch_ptr restrict bt);

QN_API extern qn_bool qn_stor_bt_add_stat_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key);
QN_API extern qn_bool qn_stor_bt_add_copy_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key);
QN_API extern qn_bool qn_stor_bt_add_move_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key);
QN_API extern qn_bool qn_stor_bt_add_delete_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key);

struct _QN_STOR_BATCH_EXTRA;
typedef struct _QN_STOR_BATCH_EXTRA * qn_stor_batch_extra_ptr;

QN_API extern qn_stor_batch_extra_ptr qn_stor_be_create(void);
QN_API extern void qn_stor_be_destroy(qn_stor_batch_extra_ptr restrict be);
QN_API extern void qn_stor_be_reset(qn_stor_batch_extra_ptr restrict be);

QN_API extern void qn_stor_be_set_region_entry(qn_stor_batch_extra_ptr restrict be, qn_rgn_entry_ptr restrict entry);

QN_API extern qn_json_object_ptr qn_stor_execute_batch_opertions(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const qn_stor_batch_ptr restrict bt, qn_stor_batch_extra_ptr restrict ext);

// ---- Declaration of Upload ----

// ---- Declaration of Put Policy ----

QN_API extern qn_json_object_ptr qn_stor_pp_create(const char * restrict bucket, const char * restrict key, qn_uint32 deadline);
QN_API extern void qn_stor_pp_destroy(qn_json_object_ptr restrict pp);

QN_API extern qn_bool qn_stor_pp_set_scope(qn_json_object_ptr restrict pp, const char * restrict bucket, const char * restrict key);
QN_API extern qn_bool qn_stor_pp_set_deadline(qn_json_object_ptr restrict pp, qn_uint32 deadline);

QN_API extern qn_bool qn_stor_pp_dont_overwrite(qn_json_object_ptr restrict pp);

QN_API extern qn_bool qn_stor_pp_return_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict body);
QN_API extern qn_bool qn_stor_pp_return_to_client(qn_json_object_ptr restrict pp, const char * restrict body);

QN_API extern qn_bool qn_stor_pp_callback_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict host_name);
QN_API extern qn_bool qn_stor_pp_callback_with_body(qn_json_object_ptr restrict pp, const char * restrict body, const char * restrict mime_type);

QN_API extern qn_bool qn_stor_pp_pfop_set_commands(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char * restrict cmd1, const char * restrict cmd2, ...);
QN_API extern qn_bool qn_stor_pp_pfop_set_command_list(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char ** restrict cmds, int cmd_count);
QN_API extern qn_bool qn_stor_pp_pfop_notify_to_server(qn_json_object_ptr restrict pp, const char * restrict url);

QN_API extern qn_bool qn_stor_pp_mime_enable_auto_detecting(qn_json_object_ptr restrict pp);
QN_API extern qn_bool qn_stor_pp_mime_allow(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...);
QN_API extern qn_bool qn_stor_pp_mime_allow_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, int mime_count);
QN_API extern qn_bool qn_stor_pp_mime_deny(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...);
QN_API extern qn_bool qn_stor_pp_mime_deny_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, int mime_count);

QN_API extern qn_bool qn_stor_pp_fsize_set_minimum(qn_json_object_ptr restrict pp, qn_uint32 min_size);
QN_API extern qn_bool qn_stor_pp_fsize_set_maximum(qn_json_object_ptr restrict pp, qn_uint32 max_size);

QN_API extern qn_bool qn_stor_pp_key_enable_fetching_from_callback_response(qn_json_object_ptr restrict pp);
QN_API extern qn_bool qn_stor_pp_key_make_from_template(qn_json_object_ptr restrict pp, const char * restrict key_template);

QN_API extern qn_bool qn_stor_pp_auto_delete_after_days(qn_json_object_ptr restrict pp, qn_uint32 days);

QN_API extern qn_bool qn_stor_pp_upload_message(qn_json_object_ptr restrict pp, const char * restrict msg_queue, const char * restrict msg_body, const char * restrict msg_mime_type);

QN_API extern qn_string qn_stor_pp_to_uptoken(qn_json_object_ptr restrict pp, qn_mac_ptr restrict mac);

// ---- 

struct _QN_STOR_PUT_EXTRA;
typedef struct _QN_STOR_PUT_EXTRA * qn_stor_put_extra_ptr;

QN_API extern qn_stor_put_extra_ptr qn_stor_pe_create(void);
QN_API extern void qn_stor_pe_destroy(qn_stor_put_extra_ptr restrict pe);
QN_API extern void qn_stor_pe_reset(qn_stor_put_extra_ptr restrict pe);

QN_API extern void qn_stor_pe_set_final_key(qn_stor_put_extra_ptr restrict pe, const char * restrict final_key);
QN_API extern void qn_stor_pe_set_local_crc32(qn_stor_put_extra_ptr restrict pe, const char * restrict crc32);
QN_API extern void qn_stor_pe_set_accept_type(qn_stor_put_extra_ptr restrict pe, const char * restrict accept_type);
QN_API extern void qn_stor_pe_set_region_entry(qn_stor_put_extra_ptr restrict pe, qn_rgn_entry_ptr restrict entry);
QN_API extern void qn_stor_pe_set_source_reader(qn_stor_put_extra_ptr restrict pe, qn_io_reader_itf restrict rdr, qn_fsize fsize);

QN_API extern qn_json_object_ptr qn_stor_put_file(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict fname, qn_stor_put_extra_ptr restrict ext);
QN_API extern qn_json_object_ptr qn_stor_put_buffer(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict buf, int buf_size, qn_stor_put_extra_ptr restrict ext);

QN_API extern qn_json_object_ptr qn_stor_upload(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict rdr, qn_stor_put_extra_ptr restrict ext);

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

QN_API extern void qn_stor_rpe_set_source_reader(qn_stor_rput_extra_ptr restrict rpe, qn_io_reader_itf restrict rdr, qn_fsize fsize);

// ----

QN_API extern qn_json_object_ptr qn_stor_rp_put_chunk(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_json_object_ptr restrict blk_info, qn_io_reader_itf restrict rdr, int chk_size, qn_stor_rput_extra_ptr restrict ext);
QN_API extern qn_json_object_ptr qn_stor_rp_put_block(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_json_object_ptr restrict blk_info, qn_io_reader_itf restrict rdr, qn_stor_rput_extra_ptr restrict ext);
QN_API extern qn_json_object_ptr qn_stor_rp_make_file(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_rput_session_ptr restrict ss, qn_stor_rput_extra_ptr restrict ext);

QN_API extern qn_json_object_ptr qn_stor_rp_put_file(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_rput_session_ptr * restrict ss, const char * restrict fname, qn_stor_rput_extra_ptr restrict ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_STORAGE_H__

