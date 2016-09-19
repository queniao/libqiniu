#include "qiniu/base/errors.h"
#include "qiniu/http.h"
#include "qiniu/region.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ---- Definition of Region Types----

typedef unsigned short int qn_rgn_pos;

// ---- Definition of Region Host ----

typedef struct _QN_RGN_HOST
{
    qn_rgn_entry * entries;
    qn_rgn_pos cnt;
    qn_rgn_pos cap;
} qn_rgn_host;

QN_API qn_rgn_host_ptr qn_rgn_host_create(void)
{
    qn_rgn_host_ptr new_host = calloc(1, sizeof(qn_rgn_host));
    if (!new_host) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_host->cap = 4;
    new_host->entries = calloc(new_host->cap, sizeof(qn_rgn_entry));
    if (!new_host->entries) {
        qn_err_set_no_enough_memory();
        free(new_host);
        return NULL;
    } // if
    return new_host;
}

QN_API void qn_rgn_host_destroy(qn_rgn_host_ptr restrict host)
{
    if (host) {
        qn_rgn_host_reset(host);
        free(host->entries);
        free(host);
    } // if
}

QN_API void qn_rgn_host_reset(qn_rgn_host_ptr restrict host)
{
    while (host->cnt > 0) {
        host->cnt -= 1;
        qn_str_destroy(host->entries[host->cnt].base_url);
        qn_str_destroy(host->entries[host->cnt].hostname);
    } // while
}

static qn_rgn_host_ptr qn_rgn_host_duplicate(qn_rgn_host_ptr restrict host)
{
    qn_rgn_pos i;
    qn_rgn_entry_ptr ent;
    qn_rgn_host_ptr new_host = qn_rgn_host_create();
    if (!new_host) return NULL;

    for (i = 0; i < host->cnt; i += 1) {
        ent = qn_rgn_host_get_entry(host, i);
        if (! qn_rgn_host_add_entry(new_host, qn_str_cstr(ent->base_url), qn_str_cstr(ent->hostname))) {
            qn_rgn_host_destroy(new_host);
            return NULL;
        } // if
    } // for
    return new_host;
}

QN_API int qn_rgn_host_entry_count(qn_rgn_host_ptr restrict host)
{
    return host->cnt;
}

static qn_bool qn_rgn_host_augment(qn_rgn_host_ptr restrict host)
{
    qn_rgn_pos new_cap = host->cap + (host->cap >> 1); // 1.5 times.
    qn_rgn_entry * new_entries = calloc(new_cap, sizeof(qn_rgn_entry));
    if (!new_entries) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_entries, host->entries, new_cap * sizeof(qn_rgn_entry));
    free(host->entries);
    host->entries = new_entries;
    host->cap = new_cap;
    return qn_true;
}

QN_API qn_bool qn_rgn_host_add_entry(qn_rgn_host_ptr restrict host, const char * restrict base_url, const char * restrict hostname)
{
    qn_rgn_entry_ptr new_ent;
    if ((host->cnt == host->cap) && !qn_rgn_host_augment(host)) return qn_false;

    new_ent = &host->entries[host->cnt];
    new_ent->base_url = qn_cs_duplicate(base_url);
    if (!new_ent->base_url) return NULL;

    new_ent->hostname = qn_cs_duplicate(hostname);
    if (!new_ent->hostname) {
        qn_str_destroy(new_ent->base_url);
        return NULL;
    } // if

    host->cnt += 1;
    return qn_true;
}

QN_API qn_rgn_entry_ptr qn_rgn_host_get_entry(qn_rgn_host_ptr restrict host, int n)
{
    return (n < host->cnt) ? &host->entries[n] : NULL;
}

// ---- Definition of Region ----

typedef struct _QN_REGION
{
    qn_string name;
    qn_rgn_host_ptr up;
    qn_rgn_host_ptr io;
    qn_rgn_host_ptr rs;
    qn_rgn_host_ptr rsf;
    qn_rgn_host_ptr api;
} qn_region, *qn_region_ptr;

QN_API qn_region_ptr qn_rgn_create(const char * restrict name)
{
    qn_region_ptr new_rgn = calloc(1, sizeof(qn_region));
    if (!new_rgn) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    if (! (new_rgn->up = qn_rgn_host_create())) goto QN_RGN_CREATE_CLEAN_FOR_UP;
    if (! (new_rgn->io = qn_rgn_host_create())) goto QN_RGN_CREATE_CLEAN_FOR_IO;
    if (! (new_rgn->rs = qn_rgn_host_create())) goto QN_RGN_CREATE_CLEAN_FOR_RS;
    if (! (new_rgn->rsf = qn_rgn_host_create())) goto QN_RGN_CREATE_CLEAN_FOR_RSF;
    if (! (new_rgn->api = qn_rgn_host_create())) goto QN_RGN_CREATE_CLEAN_FOR_API;
    return new_rgn;

QN_RGN_CREATE_CLEAN_FOR_API:
    qn_rgn_host_destroy(new_rgn->rsf);

QN_RGN_CREATE_CLEAN_FOR_RSF:
    qn_rgn_host_destroy(new_rgn->rs);

QN_RGN_CREATE_CLEAN_FOR_RS:
    qn_rgn_host_destroy(new_rgn->io);

QN_RGN_CREATE_CLEAN_FOR_IO:
    qn_rgn_host_destroy(new_rgn->up);

QN_RGN_CREATE_CLEAN_FOR_UP:
    free(new_rgn);
    return NULL;
}

static qn_region_ptr qn_rgn_clone(qn_region_ptr rgn, const char * restrict name)
{
    qn_region_ptr new_rgn = calloc(1, sizeof(qn_region));
    if (!new_rgn) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    if (! (new_rgn->name = qn_cs_duplicate(name))) goto QN_RGB_CLONE_CLEAN_FOR_NAME;

    if (! (new_rgn->up = qn_rgn_host_duplicate(rgn->up))) goto QN_RGB_CLONE_CLEAN_FOR_UP;
    if (! (new_rgn->io = qn_rgn_host_duplicate(rgn->io))) goto QN_RGB_CLONE_CLEAN_FOR_IO;
    if (! (new_rgn->rs = qn_rgn_host_duplicate(rgn->rs))) goto QN_RGB_CLONE_CLEAN_FOR_RS;
    if (! (new_rgn->rsf = qn_rgn_host_duplicate(rgn->rsf))) goto QN_RGB_CLONE_CLEAN_FOR_RSF;
    if (! (new_rgn->api = qn_rgn_host_duplicate(rgn->api))) goto QN_RGB_CLONE_CLEAN_FOR_API;

    return new_rgn;

QN_RGB_CLONE_CLEAN_FOR_API:
    qn_rgn_host_destroy(new_rgn->rsf);

QN_RGB_CLONE_CLEAN_FOR_RSF:
    qn_rgn_host_destroy(new_rgn->rs);

QN_RGB_CLONE_CLEAN_FOR_RS:
    qn_rgn_host_destroy(new_rgn->io);

QN_RGB_CLONE_CLEAN_FOR_IO:
    qn_rgn_host_destroy(new_rgn->up);

QN_RGB_CLONE_CLEAN_FOR_UP:
    qn_str_destroy(new_rgn->name);

QN_RGB_CLONE_CLEAN_FOR_NAME:
    free(new_rgn);
    return NULL;
}

QN_API void qn_rgn_destroy(qn_region_ptr restrict rgn)
{
    if (rgn) {
        qn_rgn_host_destroy(rgn->api);
        qn_rgn_host_destroy(rgn->rsf);
        qn_rgn_host_destroy(rgn->rs);
        qn_rgn_host_destroy(rgn->io);
        qn_rgn_host_destroy(rgn->up);
        qn_str_destroy(rgn->name);
        free(rgn);
    } // if
}

QN_API void qn_rgn_reset(qn_region_ptr restrict rgn)
{
    qn_rgn_host_reset(rgn->api);
    qn_rgn_host_reset(rgn->rsf);
    qn_rgn_host_reset(rgn->rs);
    qn_rgn_host_reset(rgn->io);
    qn_rgn_host_reset(rgn->up);
}

QN_API qn_rgn_host_ptr qn_rgn_get_up_host(qn_region_ptr restrict rgn)
{
    return rgn->up;
}

QN_API qn_rgn_host_ptr qn_rgn_get_io_host(qn_region_ptr restrict rgn)
{
    return rgn->io;
}

QN_API qn_rgn_host_ptr qn_rgn_get_rs_host(qn_region_ptr restrict rgn)
{
    return rgn->rs;
}

QN_API qn_rgn_host_ptr qn_rgn_get_rsf_host(qn_region_ptr restrict rgn)
{
    return rgn->rsf;
}

QN_API qn_rgn_host_ptr qn_rgn_get_api_host(qn_region_ptr restrict rgn)
{
    return rgn->api;
}

// ---- Definition of Region Table ----

typedef struct _QN_RGN_TABLE
{
    qn_region_ptr * regions;
    qn_rgn_pos cnt;
    qn_rgn_pos cap;
} qn_rgn_table;

QN_API qn_rgn_table_ptr qn_rgn_tbl_create(void)
{
    qn_rgn_table_ptr new_rtbl = calloc(1, sizeof(qn_rgn_table));
    if (!new_rtbl) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_rtbl->cap = 4;
    new_rtbl->regions = calloc(new_rtbl->cap, sizeof(qn_region_ptr));
    if (!new_rtbl->regions) {
        free(new_rtbl);
        qn_err_set_no_enough_memory();
        return NULL;
    } // if
    return new_rtbl;
}

QN_API void qn_rgn_tbl_destroy(qn_rgn_table_ptr restrict rtbl)
{
   if (rtbl) { 
        qn_rgn_tbl_reset(rtbl);
        free(rtbl->regions);
        free(rtbl);
    } // if
}

QN_API void qn_rgn_tbl_reset(qn_rgn_table_ptr restrict rtbl)
{
    while (rtbl->cnt > 0) qn_rgn_destroy(rtbl->regions[--rtbl->cnt]);
}

// ----

static qn_bool qn_rgn_tbl_augment(qn_rgn_table_ptr restrict rtbl)
{
    qn_rgn_pos new_cap = rtbl->cap + (rtbl->cap >> 1); // 1.5 times.
    qn_region_ptr * new_regions = calloc(new_cap, sizeof(qn_region_ptr));
    if (!new_regions) {
        qn_err_set_no_enough_memory();
        return qn_false;
    } // if

    memcpy(new_regions, rtbl->regions, rtbl->cnt * sizeof(qn_region_ptr));
    free(rtbl->regions);
    rtbl->regions = new_regions;
    rtbl->cap = new_cap;
    return qn_true;
}

static qn_rgn_pos qn_rgn_tbl_bsearch(qn_region_ptr * restrict regions, const qn_rgn_pos cnt, const char * restrict name)
{
    qn_rgn_pos begin = 0;
    qn_rgn_pos end = cnt;
    qn_rgn_pos mid = 0;
    while (begin < end) {
        mid = begin + ((end - begin) / 2);
        if (qn_str_compare_raw(regions[mid]->name, name) < 0) {
            begin = mid + 1;
        } else {
            end = mid;
        } // if
    } // while
    return begin;
}

QN_API const qn_region_ptr qn_rgn_tbl_get_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name)
{
    qn_rgn_pos pos = qn_rgn_tbl_bsearch(rtbl->regions, rtbl->cnt, name);
    return (pos < rtbl->cnt) ? rtbl->regions[pos] : NULL;
}

QN_API qn_bool qn_rgn_tbl_set_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name, const qn_region_ptr restrict rgn)
{
    qn_region_ptr new_rgn;
    qn_rgn_pos pos = qn_rgn_tbl_bsearch(rtbl->regions, rtbl->cnt, name);

    if (pos < rtbl->cnt) {
        // The entry with the givan name exists.
        new_rgn = qn_rgn_clone(rgn, qn_str_cstr(rtbl->regions[pos]->name));
        if (!new_rgn) return qn_false;

        qn_rgn_destroy(rtbl->regions[pos]);
        rtbl->regions[pos] = new_rgn;
        return qn_true;
    } // if

    if ((rtbl->cnt == rtbl->cap) && !qn_rgn_tbl_augment(rtbl)) return qn_false;

    if (! (rtbl->regions[pos] = qn_rgn_clone(rgn, name))) return qn_false;
    rtbl->cnt += 1;
    return qn_true;
}

static qn_rgn_entry qn_rgn_default_up_entry = { "http://up.qiniu.com", NULL };
static qn_rgn_host qn_rgn_default_up_host = { &qn_rgn_default_up_entry, 1, 1 };

static qn_rgn_entry qn_rgn_default_io_entry = { "http://iovip.qbox.me", NULL };
static qn_rgn_host qn_rgn_default_io_host = { &qn_rgn_default_io_entry, 1, 1 };

static qn_rgn_entry qn_rgn_default_rs_entry = { "http://rs.qiniu.com", NULL };
static qn_rgn_host qn_rgn_default_rs_host = { &qn_rgn_default_rs_entry, 1, 1 };

static qn_rgn_entry qn_rgn_default_rsf_entry = { "http://rsf.qbox.me", NULL };
static qn_rgn_host qn_rgn_default_rsf_host = { &qn_rgn_default_rsf_entry, 1, 1 };

static qn_rgn_entry qn_rgn_default_api_entry = { "http://api.qiniu.com", NULL };
static qn_rgn_host qn_rgn_default_api_host = { &qn_rgn_default_api_entry, 1, 1 };

static qn_region qn_rgn_default_region = {
    "default",
    &qn_rgn_default_up_host,
    &qn_rgn_default_io_host,
    &qn_rgn_default_rs_host,
    &qn_rgn_default_rsf_host,
    &qn_rgn_default_api_host
};

QN_API const qn_region_ptr qn_rgn_tbl_get_default_region(qn_rgn_table_ptr restrict rtbl)
{
    qn_region_ptr rgn = qn_rgn_tbl_get_region(rtbl, "default");
    return (rgn) ? rgn : &qn_rgn_default_region;
}

QN_API qn_bool qn_rgn_tbl_set_default_region(qn_rgn_table_ptr restrict rtbl, const qn_region_ptr restrict rgn)
{
    return qn_rgn_tbl_set_region(rtbl, "default", rgn);
}


// ---- Definition of Region Interator ----

typedef struct _QN_RGN_ITERATOR
{
    qn_rgn_table_ptr rtbl;
    qn_rgn_pos pos;
} qn_rgn_iterator;

QN_API qn_rgn_iterator_ptr qn_rgn_itr_create(qn_rgn_table_ptr restrict rtbl)
{
    qn_rgn_iterator_ptr new_itr = calloc(1, sizeof(qn_rgn_iterator));
    if (!new_itr) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_itr->rtbl = rtbl;
    new_itr->pos = 0;
    return new_itr;
}

QN_API void qn_rgn_itr_destroy(qn_rgn_iterator_ptr restrict itr)
{
    if (itr) {
        free(itr);
    } // if
}

QN_API void qn_rgn_itr_rewind(qn_rgn_iterator_ptr restrict itr)
{
    itr->pos = 0;
}

QN_API qn_bool qn_rgn_itr_next_pair(qn_rgn_iterator_ptr restrict itr, const char ** restrict name, qn_region_ptr * restrict rgn)
{
    if (itr->pos == itr->rtbl->cnt) return qn_false;
    *name = qn_str_cstr(itr->rtbl->regions[itr->pos]->name);
    *rgn = itr->rtbl->regions[itr->pos];
    return qn_true;
}

// ---- Definition of Region Service ----

typedef struct _QN_RGN_SERVICE
{
    qn_http_connection_ptr conn;
    qn_http_request_ptr req;
    qn_http_response_ptr resp;
} qn_rgn_service;

QN_API qn_rgn_service_ptr qn_rgn_svc_create(void)
{
    qn_rgn_service_ptr new_svc = calloc(1, sizeof(qn_rgn_service));
    if (!new_svc) {
        qn_err_set_no_enough_memory();
        return NULL;
    } // if

    new_svc->conn = qn_http_conn_create();
    if (!new_svc->conn) {
        free(new_svc);
        return NULL;
    } // if

    new_svc->req = qn_http_req_create();
    if (!new_svc->req) {
        qn_http_conn_destroy(new_svc->conn);
        free(new_svc);
        return NULL;
    } // if

    new_svc->resp = qn_http_resp_create();
    if (!new_svc->resp) {
        qn_http_req_destroy(new_svc->req);
        qn_http_conn_destroy(new_svc->conn);
        free(new_svc);
        return NULL;
    } // if
    return new_svc;
}

QN_API void qn_rgn_svc_destroy(qn_rgn_service_ptr restrict svc)
{
    if (svc) {
        qn_http_resp_destroy(svc->resp);
        qn_http_req_destroy(svc->req);
        qn_http_conn_destroy(svc->conn);
        free(svc);
    } // if
}

QN_API qn_bool qn_rgn_svc_grab_conf(qn_rgn_service_ptr restrict svc, qn_rgn_table_ptr restrict rtbl)
{
    return qn_true;
}


#ifdef __cplusplus
}
#endif

