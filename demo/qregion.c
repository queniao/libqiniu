#include <stdio.h>
#include <time.h>
#include "qiniu/region.h"

int main(int argc, char * argv[])
{
    const char * access_key;
    const char * bucket;
    const char * name;
    const char * base_url;
    const char * hostname;
    int i;
    qn_rgn_service_ptr rgn_svc;
    qn_rgn_table_ptr rgn_tbl;
    qn_region_ptr rgn;
    qn_rgn_host_ptr rgn_host;
    qn_rgn_entry_ptr rgn_entry;
    qn_rgn_iterator_ptr rgn_itr;
    qn_rgn_auth rgn_auth;

    if (argc < 3) {
        printf("Usage: qregion <ACCESS_KEY> <BUCKET>\n");
        return 0;
    } // if

    access_key = argv[1];
    bucket = argv[2];

    memset(&rgn_auth, 0, sizeof(rgn_auth));
    rgn_auth.server_end.access_key = access_key;

    rgn_tbl = qn_rgn_tbl_create();
    if (!rgn_tbl) {
        printf("Cannot initialize a new region table object.\n");
        return 1;
    } // if

    rgn_svc = qn_rgn_svc_create();
    if (!rgn_svc) {
        qn_rgn_tbl_destroy(rgn_tbl);
        printf("Cannot initialize a new region service object.\n");
        return 1;
    } // if

    if (!qn_rgn_svc_grab_bucket_region(rgn_svc, &rgn_auth, bucket, rgn_tbl)) {
        qn_rgn_svc_destroy(rgn_svc);
        qn_rgn_tbl_destroy(rgn_tbl);
        printf("Cannot grab the region info for the `%s` bucket\n", bucket);
        return 2;
    } // if

    rgn_itr = qn_rgn_itr_create(rgn_tbl);
    while (qn_rgn_itr_next_pair(rgn_itr, &name, &rgn)) {
        rgn_host = qn_rgn_get_up_host(rgn);
        for (i = 0; i < qn_rgn_host_entry_count(rgn_host); i += 1) {
            rgn_entry = qn_rgn_host_get_entry(rgn_host, i);
            base_url = qn_str_cstr(rgn_entry->base_url);
            hostname = qn_str_cstr(rgn_entry->hostname);
            printf("svc:[%s] host=[up] base_url:[%s] hostname:[%s]\n", name, base_url, hostname ? hostname : "");
        } // for

        rgn_host = qn_rgn_get_io_host(rgn);
        for (i = 0; i < qn_rgn_host_entry_count(rgn_host); i += 1) {
            rgn_entry = qn_rgn_host_get_entry(rgn_host, i);
            base_url = qn_str_cstr(rgn_entry->base_url);
            hostname = qn_str_cstr(rgn_entry->hostname);
            printf("svc:[%s] host=[io] base_url:[%s] hostname:[%s]\n", name, base_url, hostname ? hostname : "");
        } // for
    } // while

    qn_rgn_svc_destroy(rgn_svc);
    qn_rgn_tbl_destroy(rgn_tbl);
    return 0;
}
