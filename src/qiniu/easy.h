#ifndef __QN_EASY_H__
#define __QN_EASY_H__ 1

#include "qiniu/base/json.h"
#include "qiniu/auth.h"
#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ----

QN_API extern qn_json_object_ptr qn_easy_fi_stat(const char * restrict fname);
QN_API extern void qn_easy_fi_destroy(qn_json_object_ptr restrict fi);

QN_API extern qn_bool qn_easy_fi_compute_crc32(qn_json_object_ptr restrict fi);
QN_API extern qn_bool qn_easy_fi_compute_md5(qn_json_object_ptr restrict fi);
QN_API extern qn_bool qn_easy_fi_compute_qetag(qn_json_object_ptr restrict fi);

QN_API extern qn_bool qn_easy_fi_detect_mime_type(qn_json_object_ptr restrict fi);

// ----

struct _QN_EASY_PUT_EXTRA;
typedef struct _QN_EASY_PUT_EXTRA * qn_easy_put_extra_ptr;

QN_API extern qn_easy_put_extra_ptr qn_easy_pe_create(void);
QN_API extern void qn_easy_pe_destroy(qn_easy_put_extra_ptr restrict pe);
QN_API extern void qn_easy_pe_reset(qn_easy_put_extra_ptr restrict pe);

QN_API extern void qn_easy_pe_set_final_key(qn_easy_put_extra_ptr restrict pe, const char * restrict key);
QN_API extern void qn_easy_pe_set_owner_description(qn_easy_put_extra_ptr restrict pe, const char * restrict desc);

QN_API extern void qn_easy_pe_set_crc32_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check);
QN_API extern void qn_easy_pe_set_md5_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check);
QN_API extern void qn_easy_pe_set_qetag_checking(qn_easy_put_extra_ptr restrict pe, qn_bool check);

QN_API extern void qn_easy_pe_set_abort_variable(qn_easy_put_extra_ptr restrict pe, const volatile qn_bool * abort);

QN_API extern void qn_easy_pe_set_min_resumable_fsize(qn_easy_put_extra_ptr restrict pe, size_t fsize);

QN_API extern void qn_easy_pe_set_local_crc32(qn_easy_put_extra_ptr restrict pe, qn_uint32 crc32);
QN_API extern void qn_easy_pe_set_source_reader(qn_easy_put_extra_ptr restrict pe, qn_io_reader_itf restrict rdr, qn_fsize fsize);

QN_API extern void qn_easy_pe_set_rput_session(qn_easy_put_extra_ptr restrict pe, qn_stor_rput_session_ptr restrict rput_ss);
QN_API extern qn_stor_rput_session_ptr qn_easy_pe_get_rput_session(qn_easy_put_extra_ptr restrict pe);

// ----

struct _QN_EASY;
typedef struct _QN_EASY * qn_easy_ptr;

QN_API extern qn_easy_ptr qn_easy_create(void);
QN_API extern void qn_easy_destroy(qn_easy_ptr restrict easy);

QN_API extern qn_json_object_ptr qn_easy_put_file(qn_easy_ptr restrict easy, const char * restrict uptoken, const char * restrict fname, qn_easy_put_extra_ptr restrict ext);

#ifdef __cplusplus
}
#endif

#endif // __QN_EASY_H__

