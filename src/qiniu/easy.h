#ifndef __QN_EASY_H__
#define __QN_EASY_H__ 1

#include "qiniu/base/json.h"
#include "qiniu/auth.h"
#include "qiniu/region.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ----

struct _QN_EASY;
typedef struct _QN_EASY * qn_easy_ptr;

QN_SDK extern qn_easy_ptr qn_easy_create(void);
QN_SDK extern void qn_easy_destroy(qn_easy_ptr restrict easy);

// ----

QN_SDK extern qn_json_object_ptr qn_easy_fi_stat(const char * restrict fname);
QN_SDK extern void qn_easy_fi_destroy(qn_json_object_ptr restrict fi);

QN_SDK extern qn_bool qn_easy_fi_compute_crc32(qn_json_object_ptr restrict fi);
QN_SDK extern qn_bool qn_easy_fi_compute_md5(qn_json_object_ptr restrict fi);
QN_SDK extern qn_bool qn_easy_fi_compute_qetag(qn_json_object_ptr restrict fi);

QN_SDK extern qn_bool qn_easy_fi_detect_mime_type(qn_json_object_ptr restrict fi);

// ----

struct _QN_EASY_PUT_EXTRA;
typedef struct _QN_EASY_PUT_EXTRA * qn_easy_put_extra_ptr;

QN_SDK extern qn_easy_put_extra_ptr qn_easy_pe_create(void);
QN_SDK extern void qn_easy_pe_destroy(qn_easy_put_extra_ptr restrict pe);
QN_SDK extern void qn_easy_pe_reset(qn_easy_put_extra_ptr restrict pe);

QN_SDK extern void qn_easy_pe_set_final_key(qn_easy_put_extra_ptr restrict pe, const char * restrict key);
QN_SDK extern void qn_easy_pe_set_mime_type(qn_easy_put_extra_ptr restrict pe, const char * restrict mime_type);
QN_SDK extern void qn_easy_pe_set_owner_description(qn_easy_put_extra_ptr restrict pe, const char * restrict desc);

QN_SDK extern void qn_easy_pe_set_crc32_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check);
QN_SDK extern void qn_easy_pe_set_md5_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check);
QN_SDK extern void qn_easy_pe_set_qetag_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check);

QN_SDK extern const qn_string qn_easy_pe_get_qetag(qn_easy_put_extra_ptr restrict pe);

QN_SDK extern void qn_easy_pe_set_abort_variable(qn_easy_put_extra_ptr restrict pe, const volatile qn_bool * abort);

QN_SDK extern void qn_easy_pe_set_min_resumable_fsize(qn_easy_put_extra_ptr restrict pe, qn_size fsize);

QN_SDK extern void qn_easy_pe_set_local_crc32(qn_easy_put_extra_ptr restrict pe, qn_uint32 crc32);
QN_SDK extern void qn_easy_pe_set_source_reader(qn_easy_put_extra_ptr restrict pe, qn_io_reader_itf restrict rdr, qn_fsize fsize);

QN_SDK extern void qn_easy_pe_set_user_defined_variables(qn_easy_put_extra_ptr restrict pe, qn_ud_variable_ptr ud_vars);

QN_SDK extern qn_bool qn_easy_pe_clone_and_set_resumable_info(qn_easy_put_extra_ptr restrict pe, const char * restrict str, qn_size str_size);
QN_SDK extern qn_string qn_easy_pe_get_resumable_info(qn_easy_put_extra_ptr restrict pe);

QN_SDK extern void qn_easy_pe_set_region_host(qn_easy_put_extra_ptr restrict pe, qn_rgn_host_ptr restrict rgn_host);
QN_SDK extern void qn_easy_pe_set_region_entry(qn_easy_put_extra_ptr restrict pe, qn_rgn_entry_ptr restrict rgn_entry);

// ----

QN_SDK extern qn_json_object_ptr qn_easy_put_file(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_easy_put_extra_ptr restrict ext);

/*
QN_SDK extern qn_json_object_ptr qn_easy_put_stream(qn_easy_ptr restrict easy, const char * restrict uptoken, qn_reader_itf restrict rdr, qn_easy_put_extra_ptr restrict ext);
QN_SDK extern qn_json_object_ptr qn_easy_put_local_patches(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, const char * restrict fname_old, qn_easy_put_extra_ptr restrict ext);
QN_SDK extern qn_json_object_ptr qn_easy_put_remote_patches(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, const char * restrict bucket, const char * restrict key, qn_easy_put_extra_ptr restrict ext);
*/

// ----

struct _QN_EASY_LIST_EXTRA;
typedef struct _QN_EASY_LIST_EXTRA * qn_easy_list_extra_ptr;

QN_SDK extern qn_easy_list_extra_ptr qn_easy_le_create(void);
QN_SDK extern void qn_easy_le_destroy(qn_easy_list_extra_ptr restrict le);

QN_SDK extern void qn_easy_le_set_prefix(qn_easy_list_extra_ptr restrict le, const char * restrict prefix, const char * delimiter);
QN_SDK extern void qn_easy_le_set_limit(qn_easy_list_extra_ptr restrict le, unsigned int limit);

typedef qn_bool (*qn_easy_le_itr_callback_fn)(void * restrict user_data, qn_json_object_ptr restrict entry);

QN_SDK extern qn_json_object_ptr qn_easy_list(qn_easy_ptr restrict easy, const qn_mac_ptr restrict mac, const char * restrict bucket, void * restrict itr_data, qn_easy_le_itr_callback_fn itr_cb, qn_easy_list_extra_ptr restrict ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_EASY_H__

