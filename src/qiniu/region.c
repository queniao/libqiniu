#include <ctype.h>

#include "qiniu/base/errors.h"
#include "qiniu/http.h"
#include "qiniu/region.h"
#include "qiniu/version.h"

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

QN_SDK qn_rgn_host_ptr qn_rgn_host_create(void)
{
    qn_rgn_host_ptr new_host = calloc(1, sizeof(qn_rgn_host));
    if (!new_host) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_host->cap = 4;
    new_host->entries = calloc(new_host->cap, sizeof(qn_rgn_entry));
    if (!new_host->entries) {
        qn_err_set_out_of_memory();
        free(new_host);
        return NULL;
    } // if
    return new_host;
}

QN_SDK void qn_rgn_host_destroy(qn_rgn_host_ptr restrict host)
{
    if (host) {
        qn_rgn_host_reset(host);
        free(host->entries);
        free(host);
    } // if
}

QN_SDK void qn_rgn_host_reset(qn_rgn_host_ptr restrict host)
{
    while (host->cnt > 0) {
        host->cnt -= 1;
        qn_str_destroy(host->entries[host->cnt].base_url);
        qn_str_destroy(host->entries[host->cnt].hostname);
    } // while
}

static qn_bool qn_rgn_host_augment(qn_rgn_host_ptr restrict host)
{
    qn_rgn_pos new_cap = host->cap + (host->cap >> 1); // 1.5 times.
    qn_rgn_entry * new_entries = calloc(new_cap, sizeof(qn_rgn_entry));
    if (!new_entries) {
        qn_err_set_out_of_memory();
        return qn_false;
    } // if

    memcpy(new_entries, host->entries, new_cap * sizeof(qn_rgn_entry));
    free(host->entries);
    host->entries = new_entries;
    host->cap = new_cap;
    return qn_true;
}

static qn_bool qn_rgn_host_add_entry_raw(qn_rgn_host_ptr restrict host, const char * restrict base_url, qn_size base_url_size, const char * restrict hostname, qn_size hostname_size)
{
    qn_rgn_entry_ptr new_ent;

    if (!base_url || base_url_size == 0) return qn_false;

    if ((host->cnt == host->cap) && !qn_rgn_host_augment(host)) return qn_false;

    new_ent = &host->entries[host->cnt];
    new_ent->base_url = qn_cs_clone(base_url, base_url_size);
    if (!new_ent->base_url) return NULL;

    if (hostname && hostname_size > 0) {
        new_ent->hostname = qn_cs_clone(hostname, hostname_size);
        if (!new_ent->hostname) {
            qn_str_destroy(new_ent->base_url);
            return NULL;
        } // if
    } else {
        new_ent->hostname = NULL;
    } // if

    host->cnt += 1;
    return qn_true;
}

static qn_rgn_host_ptr qn_rgn_host_duplicate(qn_rgn_host_ptr restrict host)
{
    qn_rgn_pos i;
    qn_rgn_entry_ptr ent;
    qn_rgn_host_ptr new_host = qn_rgn_host_create();
    if (!new_host) return NULL;

    for (i = 0; i < host->cnt; i += 1) {
        ent = qn_rgn_host_get_entry(host, i);
        if (! qn_rgn_host_add_entry_raw(new_host, qn_str_cstr(ent->base_url), qn_str_size(ent->base_url), qn_str_cstr(ent->hostname), qn_str_size(ent->hostname))) {
            qn_rgn_host_destroy(new_host);
            return NULL;
        } // if
    } // for
    return new_host;
}

QN_SDK int qn_rgn_host_entry_count(qn_rgn_host_ptr restrict host)
{
    return host->cnt;
}

QN_SDK qn_bool qn_rgn_host_add_entry(qn_rgn_host_ptr restrict host, const char * restrict base_url, const char * restrict hostname)
{
    return qn_rgn_host_add_entry_raw(host, base_url, strlen(base_url), hostname, strlen(hostname));
}

QN_SDK qn_rgn_entry_ptr qn_rgn_host_get_entry(qn_rgn_host_ptr restrict host, int n)
{
    return (n < host->cnt) ? &host->entries[n] : NULL;
}

// ---- Definition of Region ----

typedef struct _QN_REGION
{
    int time_to_live;
    qn_string name;
    qn_rgn_host_ptr up;
    qn_rgn_host_ptr io;
    qn_rgn_host_ptr rs;
    qn_rgn_host_ptr rsf;
    qn_rgn_host_ptr api;
} qn_region, *qn_region_ptr;

QN_SDK qn_region_ptr qn_rgn_create(const char * restrict name)
{
    qn_region_ptr new_rgn = calloc(1, sizeof(qn_region));
    if (!new_rgn) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_rgn->time_to_live = 86400;

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
        qn_err_set_out_of_memory();
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

QN_SDK void qn_rgn_destroy(qn_region_ptr restrict rgn)
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

QN_SDK void qn_rgn_reset(qn_region_ptr restrict rgn)
{
    qn_rgn_host_reset(rgn->api);
    qn_rgn_host_reset(rgn->rsf);
    qn_rgn_host_reset(rgn->rs);
    qn_rgn_host_reset(rgn->io);
    qn_rgn_host_reset(rgn->up);
}

QN_SDK qn_rgn_host_ptr qn_rgn_get_host(qn_region_ptr restrict rgn, int svc)
{
    switch (svc) {
        case QN_RGN_SVC_UP: return rgn->up;
        case QN_RGN_SVC_IO: return rgn->io;
        case QN_RGN_SVC_RS: return rgn->rs;
        case QN_RGN_SVC_RSF: return rgn->rsf;
        case QN_RGN_SVC_API: return rgn->api;
    } // switch
    return NULL;
}

// ---- Definition of Region Table ----

typedef struct _QN_RGN_TABLE
{
    qn_region_ptr * regions;
    qn_rgn_pos cnt;
    qn_rgn_pos cap;
} qn_rgn_table;

QN_SDK qn_rgn_table_ptr qn_rgn_tbl_create(void)
{
    qn_rgn_table_ptr new_rtbl = calloc(1, sizeof(qn_rgn_table));
    if (!new_rtbl) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_rtbl->cap = 4;
    new_rtbl->regions = calloc(new_rtbl->cap, sizeof(qn_region_ptr));
    if (!new_rtbl->regions) {
        free(new_rtbl);
        qn_err_set_out_of_memory();
        return NULL;
    } // if
    return new_rtbl;
}

QN_SDK void qn_rgn_tbl_destroy(qn_rgn_table_ptr restrict rtbl)
{
   if (rtbl) { 
        qn_rgn_tbl_reset(rtbl);
        free(rtbl->regions);
        free(rtbl);
    } // if
}

QN_SDK void qn_rgn_tbl_reset(qn_rgn_table_ptr restrict rtbl)
{
    while (rtbl->cnt > 0) qn_rgn_destroy(rtbl->regions[--rtbl->cnt]);
}

// ----

static qn_bool qn_rgn_tbl_augment(qn_rgn_table_ptr restrict rtbl)
{
    qn_rgn_pos new_cap = rtbl->cap + (rtbl->cap >> 1); // 1.5 times.
    qn_region_ptr * new_regions = calloc(new_cap, sizeof(qn_region_ptr));
    if (!new_regions) {
        qn_err_set_out_of_memory();
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

QN_SDK const qn_region_ptr qn_rgn_tbl_get_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name)
{
    qn_rgn_pos pos = qn_rgn_tbl_bsearch(rtbl->regions, rtbl->cnt, name);
    return (pos < rtbl->cnt) ? rtbl->regions[pos] : NULL;
}

QN_SDK qn_bool qn_rgn_tbl_set_region(qn_rgn_table_ptr restrict rtbl, const char * restrict name, const qn_region_ptr restrict rgn)
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
    -1,
    "default",
    &qn_rgn_default_up_host,
    &qn_rgn_default_io_host,
    &qn_rgn_default_rs_host,
    &qn_rgn_default_rsf_host,
    &qn_rgn_default_api_host
};

QN_SDK const qn_region_ptr qn_rgn_tbl_get_default_region(qn_rgn_table_ptr restrict rtbl)
{
    qn_region_ptr rgn;

    if (!rtbl) return &qn_rgn_default_region;
    
    rgn= qn_rgn_tbl_get_region(rtbl, "default");
    return (rgn) ? rgn : &qn_rgn_default_region;
}

QN_SDK qn_bool qn_rgn_tbl_set_default_region(qn_rgn_table_ptr restrict rtbl, const qn_region_ptr restrict rgn)
{
    return qn_rgn_tbl_set_region(rtbl, "default", rgn);
}

QN_SDK void qn_rgn_tbl_choose_first_entry(qn_rgn_table_ptr restrict rtbl, int svc, const char * restrict name, qn_rgn_entry_ptr * restrict entry)
{
    qn_region_ptr rgn;
    qn_rgn_host_ptr host;

    if (!*entry && name) {
        rgn = qn_rgn_tbl_get_region(rtbl, name);
        if (rgn) {
            switch (svc) {
                case QN_RGN_SVC_UP: host = qn_rgn_get_up_host(rgn); break;
                case QN_RGN_SVC_IO: host = qn_rgn_get_io_host(rgn); break;
                case QN_RGN_SVC_RS: host = qn_rgn_get_rs_host(rgn); break;
                case QN_RGN_SVC_RSF: host = qn_rgn_get_rsf_host(rgn); break;
                case QN_RGN_SVC_API: host = qn_rgn_get_api_host(rgn); break;
            } // switch

            if (host) *entry = qn_rgn_host_get_entry(host, 0);
        } // if
    } // if

    if (!*entry) {
        rgn = qn_rgn_tbl_get_default_region(rtbl);
        switch (svc) {
            case QN_RGN_SVC_UP: host = qn_rgn_get_up_host(rgn); break;
            case QN_RGN_SVC_IO: host = qn_rgn_get_io_host(rgn); break;
            case QN_RGN_SVC_RS: host = qn_rgn_get_rs_host(rgn); break;
            case QN_RGN_SVC_RSF: host = qn_rgn_get_rsf_host(rgn); break;
            case QN_RGN_SVC_API: host = qn_rgn_get_api_host(rgn); break;
        } // switch

        *entry = qn_rgn_host_get_entry(host, 0);
    } // if
}

// ---- Definition of Region Interator ----

typedef struct _QN_RGN_ITERATOR
{
    qn_rgn_table_ptr rtbl;
    qn_rgn_pos pos;
} qn_rgn_iterator;

QN_SDK qn_rgn_iterator_ptr qn_rgn_itr_create(qn_rgn_table_ptr restrict rtbl)
{
    qn_rgn_iterator_ptr new_itr = calloc(1, sizeof(qn_rgn_iterator));
    if (!new_itr) {
        qn_err_set_out_of_memory();
        return NULL;
    } // if

    new_itr->rtbl = rtbl;
    new_itr->pos = 0;
    return new_itr;
}

QN_SDK void qn_rgn_itr_destroy(qn_rgn_iterator_ptr restrict itr)
{
    if (itr) {
        free(itr);
    } // if
}

QN_SDK void qn_rgn_itr_rewind(qn_rgn_iterator_ptr restrict itr)
{
    itr->pos = 0;
}

QN_SDK qn_bool qn_rgn_itr_next_pair(qn_rgn_iterator_ptr restrict itr, const char ** restrict name, qn_region_ptr * restrict rgn)
{
    if (itr->pos == itr->rtbl->cnt) return qn_false;
    *name = qn_str_cstr(itr->rtbl->regions[itr->pos]->name);
    *rgn = itr->rtbl->regions[itr->pos];
    itr->pos += 1;
    return qn_true;
}

// ---- Definition of Region Service ----

typedef struct _QN_RGN_SERVICE
{
    qn_http_connection_ptr conn;
    qn_http_request_ptr req;
    qn_http_response_ptr resp;
    qn_http_json_writer_ptr resp_json_wrt;
} qn_rgn_service;

QN_SDK qn_rgn_service_ptr qn_rgn_svc_create(void)
{
    qn_rgn_service_ptr new_svc = calloc(1, sizeof(qn_rgn_service));
    if (!new_svc) {
        qn_err_set_out_of_memory();
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

    new_svc->resp_json_wrt = qn_http_json_wrt_create();
    if (!new_svc->resp_json_wrt) {
        qn_http_resp_destroy(new_svc->resp);
        qn_http_req_destroy(new_svc->req);
        qn_http_conn_destroy(new_svc->conn);
        free(new_svc);
        return NULL;
    } // if
    return new_svc;
}

QN_SDK void qn_rgn_svc_destroy(qn_rgn_service_ptr restrict svc)
{
    if (svc) {
        qn_http_json_wrt_destroy(svc->resp_json_wrt);
        qn_http_resp_destroy(svc->resp);
        qn_http_req_destroy(svc->req);
        qn_http_conn_destroy(svc->conn);
        free(svc);
    } // if
}

static qn_bool qn_rgn_svc_parse_and_add_entry(qn_string txt, qn_rgn_host_ptr host)
{
    const char * end;
    const char * base_url = NULL;
    qn_size base_url_size = 0;
    const char * hostname = NULL;
    qn_size hostname_size = 0;

    if ((hostname = strstr(qn_str_cstr(txt), "-H"))) {
        hostname += 2;
        while (isspace(*hostname)) hostname += 1;
        end = strchr(hostname, ' ');
        hostname_size = end - hostname;

        base_url = strstr(end, "http");
        base_url_size = qn_str_cstr(txt) + qn_str_size(txt) - hostname;
    } else {
        base_url = qn_str_cstr(txt);
        while (isspace(*base_url)) base_url++;

        end = qn_str_cstr(txt) + qn_str_size(txt);
        while (isspace(end[-1])) end -= 1;

        base_url_size = end - base_url;
    } // if

    return qn_rgn_host_add_entry_raw(host, base_url, base_url_size, hostname, hostname_size);
}

static qn_bool qn_rgn_svc_extract_and_add_entries(qn_json_object_ptr root, qn_region_ptr rgn)
{
    qn_bool ret;
    qn_rgn_host_ptr up;
    qn_rgn_host_ptr io;
    qn_json_object_ptr scheme_table;
    qn_json_array_ptr entry_list;
    int i;

    rgn->time_to_live = qn_json_get_integer(root, "ttl", 86400);

    up = qn_rgn_get_up_host(rgn);
    io = qn_rgn_get_io_host(rgn);

    scheme_table = qn_json_get_object(root, "http", NULL);
    if (scheme_table) {
        entry_list = qn_json_get_array(scheme_table, "io", NULL);
        for (i = 0; entry_list && i < qn_json_size_array(entry_list); i += 1) {
            ret = qn_rgn_svc_parse_and_add_entry(qn_json_pick_string(entry_list, i, NULL), io);
            if (!ret) return qn_false;
        } // for

        entry_list = qn_json_get_array(scheme_table, "up", NULL);
        for (i = 0; entry_list && i < qn_json_size_array(entry_list); i += 1) {
            ret = qn_rgn_svc_parse_and_add_entry(qn_json_pick_string(entry_list, i, NULL), up);
            if (!ret) return qn_false;
        } // for
    } // if

    scheme_table = qn_json_get_object(root, "https", NULL);
    if (scheme_table) {
        entry_list = qn_json_get_array(scheme_table, "io", NULL);
        for (i = 0; entry_list && i < qn_json_size_array(entry_list); i += 1) {
            ret = qn_rgn_svc_parse_and_add_entry(qn_json_pick_string(entry_list, i, NULL), io);
            if (!ret) return qn_false;
        } // for

        entry_list = qn_json_get_array(scheme_table, "up", NULL);
        for (i = 0; entry_list && i < qn_json_size_array(entry_list); i += 1) {
            ret = qn_rgn_svc_parse_and_add_entry(qn_json_pick_string(entry_list, i, NULL), up);
            if (!ret) return qn_false;
        } // for
    } // if
    return qn_true;
}

QN_SDK qn_bool qn_rgn_svc_grab_bucket_region(qn_rgn_service_ptr restrict svc, qn_rgn_auth_ptr restrict auth, const char * restrict bucket, qn_rgn_table_ptr restrict rtbl)
{
    qn_bool ret;
    qn_string url;
    qn_string encoded_bucket;
    qn_json_object_ptr root;
    qn_region_ptr new_rgn;

    // ---- Prepare the query URL
    encoded_bucket = qn_cs_percent_encode(bucket, strlen(bucket));
    if (!encoded_bucket) return qn_false;

    url = qn_cs_sprintf("%s/v1/query?ak=%s&bucket=%s", "http://uc.qbox.me", auth->server_end.access_key, qn_str_cstr(encoded_bucket));
    qn_str_destroy(encoded_bucket);
    if (!url) return qn_false;

    // ---- Prepare the request and response object
    qn_http_req_reset(svc->req);
    qn_http_resp_reset(svc->resp);

    if (!qn_http_req_set_header(svc->req, "Expect", "")) {
        qn_str_destroy(url);
        return qn_false;
    } // if
    if (!qn_http_req_set_header(svc->req, "Transfer-Encoding", "")) {
        qn_str_destroy(url);
        return qn_false;
    } // if
    if (! qn_http_req_set_header(svc->req, "User-Agent", qn_ver_get_full_string())) {
        qn_str_destroy(url);
        return qn_false;
    } // if

    qn_http_req_set_body_data(svc->req, "", 0);

    root = NULL;
    qn_http_json_wrt_prepare(svc->resp_json_wrt, &root, NULL);
    qn_http_resp_set_data_writer(svc->resp, svc->resp_json_wrt, &qn_http_json_wrt_write_cfn);

    ret = qn_http_conn_get(svc->conn, url, svc->req, svc->resp);
    qn_str_destroy(url);

    // ---- Grab the region info of the givan bucket

    if (ret) {
        if (! (new_rgn = qn_rgn_create(bucket))) {
            qn_json_destroy_object(root);
            return qn_false;
        } // if

        if (!qn_rgn_svc_extract_and_add_entries(root, new_rgn)) {
            qn_rgn_destroy(new_rgn);
            qn_json_destroy_object(root);
            return qn_false;
        } // if

        ret = qn_rgn_tbl_set_region(rtbl, bucket, new_rgn);
        qn_rgn_destroy(new_rgn);
    } // if
    // TODO: Deal with the case that API return no value.
    qn_json_destroy_object(root);
    return ret;
}

#ifdef __cplusplus
}
#endif

