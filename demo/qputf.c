#include <stdio.h>
#include <time.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/region.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_bool ret;
    const char * bucket;
    const char * key;
    const char * fname;
    qn_mac_ptr mac;
    qn_string scope;
    qn_string hdr_ent;
    qn_string put_ret_str;
    qn_json_object_ptr put_ret;
    qn_storage_ptr stor;
    qn_stor_auth auth;
    qn_stor_put_extra ext;
    qn_rgn_table_ptr rgn_tbl;
    qn_rgn_auth rgn_auth;
    qn_rgn_service_ptr rgn_svc;
    qn_http_hdr_iterator_ptr hdr_itr;

    if (argc < 5) {
        printf("Demo qputf - Put single file in one HTTP session.\n");
        printf("Usage: qputf <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <FNAME> [MSG_QUEUE MSG_BODY MSG_MIME_TYPE]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    bucket = argv[3];
    key = argv[4];
    fname = argv[5];

    memset(&rgn_auth, 0, sizeof(rgn_auth));
    rgn_auth.server_end.access_key = argv[1];

    rgn_tbl = qn_rgn_tbl_create();
    if (!rgn_tbl) {
        printf("Cannot create a region table.\n");
        return 1;
    } // if

    rgn_svc = qn_rgn_svc_create();
    if (!rgn_svc) {
        qn_rgn_tbl_destroy(rgn_tbl);
        printf("Cannot create a region service object.\n");
        return 1;
    } // if

    ret = qn_rgn_svc_grab_bucket_region(rgn_svc, &rgn_auth, bucket, rgn_tbl);
    qn_rgn_svc_destroy(rgn_svc);
    if (!ret) {
        qn_rgn_tbl_destroy(rgn_tbl);
        printf("Cannot fetch region information of the bucket `%s`.\n", bucket);
        return 1;
    } // if

    memset(&ext, 0, sizeof(ext));
    ext.final_key = key;
    qn_rgn_tbl_choose_first_entry(rgn_tbl, QN_RGN_SVC_UP, bucket, &ext.rgn.entry);

    memset(&auth, 0, sizeof(auth));
    auth.server_end.mac = mac;
    auth.server_end.put_policy = qn_json_create_object();
    if (!auth.server_end.put_policy) {
        qn_mac_destroy(mac);
        qn_rgn_tbl_destroy(rgn_tbl);
        printf("Cannot create a put policy.\n");
        return 1;
    } // if

    scope = qn_cs_sprintf("%s:%s", bucket, key);
    if (!scope) {
        qn_json_destroy_object(auth.server_end.put_policy);
        qn_mac_destroy(mac);
        qn_rgn_tbl_destroy(rgn_tbl);
        printf("Cannot format a valid scope for inserting the file.\n");
        return 1;
    } // if

    if (!qn_json_set_string(auth.server_end.put_policy, "scope", qn_str_cstr(scope))) {
        qn_str_destroy(scope);
        qn_json_destroy_object(auth.server_end.put_policy);
        qn_mac_destroy(mac);
        qn_rgn_tbl_destroy(rgn_tbl);
        printf("Cannot set the scope field.\n");
        return 1;
    } // if
    qn_str_destroy(scope);

    if (!qn_json_set_integer(auth.server_end.put_policy, "deadline", time(NULL) + 3600)) {
        qn_json_destroy_object(auth.server_end.put_policy);
        qn_mac_destroy(mac);
        qn_rgn_tbl_destroy(rgn_tbl);
        printf("Cannot set the deadline field.\n");
        return 1;
    } // if

    if (argc > 6) {
        if (!qn_json_set_string(auth.server_end.put_policy, "notifyQueue", argv[6])) {
            qn_json_destroy_object(auth.server_end.put_policy);
            qn_mac_destroy(mac);
            qn_rgn_tbl_destroy(rgn_tbl);
            printf("Cannot set the deadline field.\n");
            return 1;
        } // if

        if (!qn_json_set_string(auth.server_end.put_policy, "notifyMessage", argv[7])) {
            qn_json_destroy_object(auth.server_end.put_policy);
            qn_mac_destroy(mac);
            qn_rgn_tbl_destroy(rgn_tbl);
            printf("Cannot set the deadline field.\n");
            return 1;
        } // if

        if (!qn_json_set_string(auth.server_end.put_policy, "notifyMessageType", argv[8])) {
            qn_json_destroy_object(auth.server_end.put_policy);
            qn_mac_destroy(mac);
            qn_rgn_tbl_destroy(rgn_tbl);
            printf("Cannot set the deadline field.\n");
            return 1;
        } // if
    } // if

    stor = qn_stor_create();
    if (!stor) {
        qn_json_destroy_object(auth.server_end.put_policy);
        qn_mac_destroy(mac);
        qn_rgn_tbl_destroy(rgn_tbl);
        printf("Cannot initialize a new storage object.\n");
        return 1;
    } // if

    put_ret = qn_stor_put_file(stor, &auth, fname, &ext);
    qn_json_destroy_object(auth.server_end.put_policy);
    qn_mac_destroy(mac);
    qn_rgn_tbl_destroy(rgn_tbl);

    if (!put_ret) {
        qn_stor_destroy(stor);
        printf("Cannot put the file `%s` to `%s:%s`.\n", fname, bucket, key);
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) {
        printf("%s\n", qn_str_cstr(hdr_ent));
    } // while
    qn_http_hdr_itr_destroy(hdr_itr);

    put_ret_str = qn_json_object_to_string(put_ret);
    qn_stor_destroy(stor);
    if (!put_ret_str) {
        printf("Cannot format the object body from upload interface.\n");
        return 3;
    } // if

    printf("%s\n", put_ret_str);
    qn_str_destroy(put_ret_str);

    return 0;
}
