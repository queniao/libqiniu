#ifndef __QN_REGION_H__
#define __QN_REGION_H__

#include "qiniu/os/types.h"
#include "qiniu/base/string.h"

#include "qiniu/macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Declaration of Region Host ----

typedef struct _QN_RGN_ENTRY
{
    qn_string base_url;
    qn_string hostname;
} qn_rgn_entry, *qn_rgn_entry_ptr;

struct _QN_RGN_HOST;
typedef struct _QN_RGN_HOST * qn_rgn_host_ptr;

QN_API extern qn_rgn_host_ptr qn_rgn_host_create(void);
QN_API extern void qn_rgn_host_destroy(qn_rgn_host_ptr restrict host);
QN_API extern void qn_rgn_host_reset(qn_rgn_host_ptr restrict host);

QN_API extern int qn_rgn_host_entry_count(qn_rgn_host_ptr restrict host);

QN_API extern qn_bool qn_rgn_host_add_entry(qn_rgn_host_ptr restrict host, const char * restrict base_url, const char * restrict hostname);
QN_API extern qn_rgn_entry_ptr qn_rgn_host_get_entry(qn_rgn_host_ptr restrict host, int n);

// ---- Declaration of Region ----

struct _QN_REGION;
typedef struct _QN_REGION * qn_region_ptr;

QN_API extern qn_region_ptr qn_rgn_create(const char * restrict name);
QN_API extern void qn_rgn_destroy(qn_region_ptr restrict rgn);
QN_API extern void qn_rgn_reset(qn_region_ptr restrict rgn);

QN_API extern qn_rgn_host_ptr qn_rgn_get_up_host(qn_region_ptr restrict rgn);
QN_API extern qn_rgn_host_ptr qn_rgn_get_io_host(qn_region_ptr restrict rgn);
QN_API extern qn_rgn_host_ptr qn_rgn_get_rs_host(qn_region_ptr restrict rgn);
QN_API extern qn_rgn_host_ptr qn_rgn_get_rsf_host(qn_region_ptr restrict rgn);
QN_API extern qn_rgn_host_ptr qn_rgn_get_api_host(qn_region_ptr restrict rgn);

// ---- Declaration of Region Table ----

struct _QN_RGN_TABLE;
typedef struct _QN_RGN_TABLE * qn_rgn_table_ptr;

QN_API extern qn_rgn_table_ptr qn_rgn_tbl_create(void);
QN_API extern void qn_rgn_tbl_destroy(qn_rgn_table_ptr restrict rtbl);
QN_API extern void qn_rgn_tbl_reset(qn_rgn_table_ptr restrict rtbl);

// ----

QN_API extern const qn_region_ptr qn_rgn_tbl_get_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name);
QN_API extern qn_bool qn_rgn_tbl_set_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name, const qn_region_ptr restrict rgn);

QN_API extern const qn_region_ptr qn_rgn_tbl_get_default_region(qn_rgn_table_ptr restrict rtbl);
QN_API extern qn_bool qn_rgn_tbl_set_default_region(qn_rgn_table_ptr restrict rtbl, const qn_region_ptr restrict region);

// ---- Declaration of Region Interator ----

struct _QN_RGN_ITERATOR;
typedef struct _QN_RGN_ITERATOR * qn_rgn_iterator_ptr;

QN_API extern qn_rgn_iterator_ptr qn_rgn_itr_create(qn_rgn_table_ptr restrict rtbl);
QN_API extern void qn_rgn_itr_destroy(qn_rgn_iterator_ptr restrict itr);
QN_API extern void qn_rgn_itr_rewind(qn_rgn_iterator_ptr restrict itr);

QN_API extern qn_bool qn_rgn_itr_next_pair(qn_rgn_iterator_ptr restrict itr, const char ** restrict name, qn_region_ptr * restrict rgn);

// ---- Declaration of Region Service ----

typedef struct _QN_RGN_AUTH
{
    struct {
        const char * access_key;
    } server_end;
} qn_rgn_auth, *qn_rgn_auth_ptr;

struct _QN_RGN_SERVICE;
typedef struct _QN_RGN_SERVICE * qn_rgn_service_ptr;

QN_API extern qn_rgn_service_ptr qn_rgn_svc_create(void);
QN_API extern void qn_rgn_svc_destroy(qn_rgn_service_ptr restrict svc);

QN_API extern qn_bool qn_rgn_svc_grab_bucket_region(qn_rgn_service_ptr restrict svc, qn_rgn_auth_ptr restrict auth, const char * restrict bucket, qn_rgn_table_ptr restrict rtbl);

#ifdef __cplusplus
}
#endif

#endif // __QN_REGION_H__

