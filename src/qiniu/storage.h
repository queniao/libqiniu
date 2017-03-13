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
#include "qiniu/os/file.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// -------- Storage Object (abbreviation: stor) --------

struct _QN_STORAGE;
typedef struct _QN_STORAGE * qn_storage_ptr;

QN_SDK extern qn_storage_ptr qn_stor_create(void);
QN_SDK extern void qn_stor_destroy(qn_storage_ptr restrict stor);

QN_SDK extern qn_json_object_ptr qn_stor_get_object_body(const qn_storage_ptr restrict stor);
QN_SDK extern qn_json_array_ptr qn_stor_get_array_body(const qn_storage_ptr restrict stor);
QN_SDK extern qn_http_hdr_iterator_ptr qn_stor_resp_get_header_iterator(const qn_storage_ptr restrict stor);

// -------- Management Extra (abbreviation: mne) --------

struct _QN_STOR_MANAGEMENT_EXTRA;
typedef struct _QN_STOR_MANAGEMENT_EXTRA * qn_stor_management_extra_ptr;

QN_SDK extern qn_stor_management_extra_ptr qn_stor_mne_create(void);
QN_SDK extern void qn_stor_mne_destroy(qn_stor_management_extra_ptr restrict me);
QN_SDK extern void qn_stor_mne_reset(qn_stor_management_extra_ptr restrict me);

QN_SDK extern void qn_stor_mne_set_force_overwrite(qn_stor_management_extra_ptr restrict me, qn_bool force);
QN_SDK extern void qn_stor_mne_set_region_entry(qn_stor_management_extra_ptr restrict me, qn_rgn_entry_ptr restrict entry);

// -------- Management Functions (abbreviation: mn) --------

QN_SDK extern qn_json_object_ptr qn_stor_mn_api_stat(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_management_extra_ptr restrict mne);
QN_SDK extern qn_json_object_ptr qn_stor_mn_api_copy(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_management_extra_ptr restrict mne);
QN_SDK extern qn_json_object_ptr qn_stor_mn_api_move(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_management_extra_ptr restrict mne);
QN_SDK extern qn_json_object_ptr qn_stor_mn_api_delete(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, qn_stor_management_extra_ptr restrict mne);
QN_SDK extern qn_json_object_ptr qn_stor_mn_api_chgm(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, const char * restrict key, const char * restrict mime, qn_stor_management_extra_ptr restrict mne);

// -------- Batch Operations (abbreviation: bt) --------

struct _QN_STOR_BATCH;
typedef struct _QN_STOR_BATCH * qn_stor_batch_ptr;

QN_SDK extern qn_stor_batch_ptr qn_stor_bt_create(void);
QN_SDK extern void qn_stor_bt_destroy(qn_stor_batch_ptr restrict bt);
QN_SDK extern void qn_stor_bt_reset(qn_stor_batch_ptr restrict bt);

QN_SDK extern qn_bool qn_stor_bt_add_stat_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key);
QN_SDK extern qn_bool qn_stor_bt_add_copy_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key);
QN_SDK extern qn_bool qn_stor_bt_add_move_op(qn_stor_batch_ptr restrict bt, const char * restrict src_bucket, const char * restrict src_key, const char * restrict dest_bucket, const char * restrict dest_key);
QN_SDK extern qn_bool qn_stor_bt_add_delete_op(qn_stor_batch_ptr restrict bt, const char * restrict bucket, const char * restrict key);

// -------- Batch Functions (abbreviation: bt) --------

QN_SDK extern qn_json_object_ptr qn_stor_bt_api_batch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const qn_stor_batch_ptr restrict bt, qn_stor_management_extra_ptr restrict mne);

// -------- List Extra (abbreviation: lse) --------

struct _QN_STOR_LIST_EXTRA;
typedef struct _QN_STOR_LIST_EXTRA * qn_stor_list_extra_ptr;

QN_SDK extern qn_stor_list_extra_ptr qn_stor_lse_create(void);
QN_SDK extern void qn_stor_lse_destroy(qn_stor_list_extra_ptr restrict lse);
QN_SDK extern void qn_stor_lse_reset(qn_stor_list_extra_ptr restrict lse);

QN_SDK extern void qn_stor_lse_set_prefix(qn_stor_list_extra_ptr restrict lse, const char * restrict prefix, const char * restrict delimiter);
QN_SDK extern void qn_stor_lse_set_marker(qn_stor_list_extra_ptr restrict lse, const char * restrict marker);
QN_SDK extern void qn_stor_lse_set_limit(qn_stor_list_extra_ptr restrict lse, qn_uint32 limit);

// -------- List Functions (abbreviation: ls) --------

QN_SDK extern qn_json_object_ptr qn_stor_ls_api_list(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict bucket, qn_stor_list_extra_ptr restrict lse);

// -------- Fetch Extra (abbreviation: fte) --------

struct _QN_STOR_FETCH_EXTRA;
typedef struct _QN_STOR_FETCH_EXTRA * qn_stor_fetch_extra_ptr;

QN_SDK extern qn_stor_fetch_extra_ptr qn_stor_fte_create(void);
QN_SDK extern void qn_stor_fte_destroy(qn_stor_fetch_extra_ptr restrict fte);
QN_SDK extern void qn_stor_fte_reset(qn_stor_fetch_extra_ptr restrict fte);

QN_SDK extern void qn_stor_fte_set_region_entry(qn_stor_fetch_extra_ptr restrict fte, qn_rgn_entry_ptr restrict entry);

// -------- Fetch Functions (abbreviation: ft) --------

QN_SDK extern qn_json_object_ptr qn_stor_ft_api_fetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict src_url, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict fte);
QN_SDK extern qn_json_object_ptr qn_stor_ft_api_prefetch(qn_storage_ptr restrict stor, const qn_mac_ptr restrict mac, const char * restrict dest_bucket, const char * restrict dest_key, qn_stor_fetch_extra_ptr restrict fte);

// -------- Put Policy (abbreviation: pp) --------

QN_SDK extern qn_json_object_ptr qn_stor_pp_create(const char * restrict bucket, const char * restrict key, qn_json_integer deadline);
QN_SDK extern void qn_stor_pp_destroy(qn_json_object_ptr restrict pp);

QN_SDK extern qn_bool qn_stor_pp_set_scope(qn_json_object_ptr restrict pp, const char * restrict bucket, const char * restrict key);
QN_SDK extern qn_bool qn_stor_pp_set_deadline(qn_json_object_ptr restrict pp, qn_json_integer deadline);

QN_SDK extern qn_bool qn_stor_pp_dont_overwrite(qn_json_object_ptr restrict pp);

QN_SDK extern qn_bool qn_stor_pp_return_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict body);
QN_SDK extern qn_bool qn_stor_pp_return_to_client(qn_json_object_ptr restrict pp, const char * restrict body);

QN_SDK extern qn_bool qn_stor_pp_callback_to_server(qn_json_object_ptr restrict pp, const char * restrict url, const char * restrict host_name);
QN_SDK extern qn_bool qn_stor_pp_callback_with_body(qn_json_object_ptr restrict pp, const char * restrict body, const char * restrict mime_type);

QN_SDK extern qn_bool qn_stor_pp_pfop_set_commands(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char * restrict cmd1, const char * restrict cmd2, ...);
QN_SDK extern qn_bool qn_stor_pp_pfop_set_command_list(qn_json_object_ptr restrict pp, const char * restrict pipeline, const char ** restrict cmds, qn_uint cmd_count);
QN_SDK extern qn_bool qn_stor_pp_pfop_notify_to_server(qn_json_object_ptr restrict pp, const char * restrict url);

QN_SDK extern qn_bool qn_stor_pp_mime_enable_auto_detecting(qn_json_object_ptr restrict pp);
QN_SDK extern qn_bool qn_stor_pp_mime_allow(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...);
QN_SDK extern qn_bool qn_stor_pp_mime_allow_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, qn_uint mime_count);
QN_SDK extern qn_bool qn_stor_pp_mime_deny(qn_json_object_ptr restrict pp, const char * restrict mime1, const char * restrict mime2, ...);
QN_SDK extern qn_bool qn_stor_pp_mime_deny_list(qn_json_object_ptr restrict pp, const char ** restrict mime_list, qn_uint mime_count);

QN_SDK extern qn_bool qn_stor_pp_fsize_set_minimum(qn_json_object_ptr restrict pp, qn_json_integer min_size);
QN_SDK extern qn_bool qn_stor_pp_fsize_set_maximum(qn_json_object_ptr restrict pp, qn_json_integer max_size);

QN_SDK extern qn_bool qn_stor_pp_key_enable_fetching_from_callback_response(qn_json_object_ptr restrict pp);
QN_SDK extern qn_bool qn_stor_pp_key_make_from_template(qn_json_object_ptr restrict pp, const char * restrict key_template);

QN_SDK extern qn_bool qn_stor_pp_auto_delete_after_days(qn_json_object_ptr restrict pp, qn_json_integer days);

QN_SDK extern qn_bool qn_stor_pp_upload_message(qn_json_object_ptr restrict pp, const char * restrict msg_queue, const char * restrict msg_body, const char * restrict msg_mime_type);

QN_SDK extern qn_string qn_stor_pp_to_uptoken(qn_json_object_ptr restrict pp, qn_mac_ptr restrict mac);

// -------- Upload Extra (abbreviation: upe) --------

struct _QN_STOR_UPLOAD_EXTRA;
typedef struct _QN_STOR_UPLOAD_EXTRA * qn_stor_upload_extra_ptr;

QN_SDK extern qn_stor_upload_extra_ptr qn_stor_upe_create(void);
QN_SDK extern void qn_stor_upe_destroy(qn_stor_upload_extra_ptr restrict upe);
QN_SDK extern void qn_stor_upe_reset(qn_stor_upload_extra_ptr restrict upe);

QN_SDK extern void qn_stor_upe_set_final_key(qn_stor_upload_extra_ptr restrict upe, const char * restrict final_key);
QN_SDK extern void qn_stor_upe_set_local_crc32(qn_stor_upload_extra_ptr restrict upe, const char * restrict crc32);
QN_SDK extern void qn_stor_upe_set_accept_type(qn_stor_upload_extra_ptr restrict upe, const char * restrict accept_type);
QN_SDK extern void qn_stor_upe_set_region_entry(qn_stor_upload_extra_ptr restrict upe, qn_rgn_entry_ptr restrict entry);

// -------- Ordinary Upload (abbreviation: up) --------

QN_SDK extern qn_json_object_ptr qn_stor_up_api_upload_file(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict fname, qn_stor_upload_extra_ptr restrict upe);
QN_SDK extern qn_json_object_ptr qn_stor_up_api_upload_buffer(qn_storage_ptr restrict stor, const char * restrict uptoken, const char * restrict buf, qn_size buf_size, qn_stor_upload_extra_ptr restrict upe);

QN_SDK extern qn_json_object_ptr qn_stor_up_api_upload(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_stor_upload_extra_ptr restrict upe);

// -------- Resumable Upload Object (abbreviation: ru) --------

enum
{
    QN_STOR_RU_CHUNK_DEFAULT_SIZE = (1024 * 256),
    QN_STOR_RU_BLOCK_MAX_SIZE = (1024 * 1024 * 4),
    QN_STOR_RU_BLOCK_LAST_INDEX = (-1)
};

struct _QN_STOR_RESUMABLE_UPLOAD;
typedef struct _QN_STOR_RESUMABLE_UPLOAD * qn_stor_resumable_upload_ptr;

QN_SDK extern qn_stor_resumable_upload_ptr qn_stor_ru_create(qn_io_reader_itf restrict data_rdr);
QN_SDK extern void qn_stor_ru_destroy(qn_stor_resumable_upload_ptr restrict ru);

QN_SDK extern qn_string qn_stor_ru_to_string(qn_stor_resumable_upload_ptr restrict ru);
QN_SDK extern qn_stor_resumable_upload_ptr qn_stor_ru_from_string(const char * restrict str, qn_size str_len);

QN_SDK extern qn_bool qn_stor_ru_attach(qn_stor_resumable_upload_ptr restrict ru, qn_io_reader_itf restrict data_rdr);

QN_SDK extern int qn_stor_ru_get_block_count(qn_stor_resumable_upload_ptr restrict ru);

QN_SDK extern qn_json_object_ptr qn_stor_ru_get_block_info(qn_stor_resumable_upload_ptr restrict ru, int blk_idx);
QN_SDK extern qn_json_object_ptr qn_stor_ru_update_block_info(qn_stor_resumable_upload_ptr restrict ru, int blk_idx, qn_json_object_ptr restrict up_ret);

QN_SDK extern qn_io_reader_itf qn_stor_ru_create_block_reader(qn_stor_resumable_upload_ptr restrict ru, int blk_idx, qn_json_object_ptr * restrict blk_info);
QN_SDK extern qn_io_reader_itf qn_stor_ru_to_context_reader(qn_stor_resumable_upload_ptr restrict ru);

QN_SDK extern qn_fsize qn_stor_ru_total_fsize(qn_stor_resumable_upload_ptr restrict ru);
QN_SDK extern qn_fsize qn_stor_ru_uploaded_fsize(qn_stor_resumable_upload_ptr restrict ru);

QN_SDK extern qn_bool qn_stor_ru_is_block_uploaded(qn_json_object_ptr restrict blk_info);
QN_SDK extern qn_bool qn_stor_ru_is_file_uploaded(qn_stor_resumable_upload_ptr restrict ru);

// -------- Resumable Upload Functions (abbreviation: ru) --------

QN_SDK extern qn_json_object_ptr qn_stor_ru_api_mkblk(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict blk_info, qn_uint chk_size, qn_stor_upload_extra_ptr restrict upe);
QN_SDK extern qn_json_object_ptr qn_stor_ru_api_bput(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict data_rdr, qn_json_object_ptr restrict blk_info, qn_uint chk_size, qn_stor_upload_extra_ptr restrict upe);
QN_SDK extern qn_json_object_ptr qn_stor_ru_api_mkfile(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_io_reader_itf restrict ctx_rdr, qn_json_object_ptr restrict last_blk_info, qn_fsize fsize, qn_stor_upload_extra_ptr restrict upe);

QN_SDK extern qn_json_object_ptr qn_stor_ru_upload_huge(qn_storage_ptr restrict stor, const char * restrict uptoken, qn_stor_resumable_upload_ptr ru, int * start_idx, qn_uint chk_size, qn_stor_upload_extra_ptr restrict upe);

#ifdef __cplusplus
}
#endif

#endif // __QN_STORAGE_H__

