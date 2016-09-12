#include <stdio.h>
#include <time.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string scope = NULL;
    qn_string put_ret = NULL;
    qn_string bucket = NULL;
    qn_string key = NULL;
    qn_string fname = NULL;
    qn_storage_ptr stor = NULL;
    qn_stor_put_extra ext;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 5) {
        printf("Usage: qput <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <FNAME>\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    bucket = argv[3];
    key = argv[4];
    fname = argv[5];

    memset(&ext, 0, sizeof(ext));
    ext.final_key = key;
    ext.server_end.mac = mac;
    ext.server_end.put_policy = qn_json_create_object();
    if (!ext.server_end.put_policy) {
        printf("Cannot create a put policy.\n");
        return 1;
    } // if

    scope = qn_str_sprintf("%s:%s", bucket, key);
    if (!scope) {
        qn_json_destroy_object(ext.server_end.put_policy);
        printf("Cannot format a valid scope for inserting the file.\n");
        return 1;
    } // if

    if (!qn_json_set_string(ext.server_end.put_policy, "scope", qn_str_cstr(scope))) {
        qn_str_destroy(scope);
        qn_json_destroy_object(ext.server_end.put_policy);
        printf("Cannot set the scope field.\n");
        return 1;
    } // if
    qn_str_destroy(scope);

    if (!qn_json_set_integer(ext.server_end.put_policy, "deadline", time(NULL) + 3600)) {
        qn_str_destroy(scope);
        qn_json_destroy_object(ext.server_end.put_policy);
        printf("Cannot set the deadline field.\n");
        return 1;
    } // if

    stor = qn_stor_create();
    if (!stor) {
        qn_json_destroy_object(ext.server_end.put_policy);
        printf("Cannot initialize a new storage object.\n");
        return 1;
    } // if

    if (!qn_stor_put_file(stor, fname, &ext)) {
        qn_stor_destroy(stor);
        qn_json_destroy_object(ext.server_end.put_policy);
        printf("Cannot put the file `%s` to `%s:%s`.\n", fname, bucket, key);
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) {
        printf("%s\n", qn_str_cstr(hdr_ent));
    } // while

    put_ret = qn_json_object_to_string(qn_stor_get_object_body(stor));
    if (!put_ret) {
        qn_stor_destroy(stor);
        qn_json_destroy_object(ext.server_end.put_policy);
        printf("Cannot format the object body from upload interface.\n");
        return 3;
    } // if

    printf("%s\n", put_ret);
    qn_str_destroy(put_ret);

    qn_stor_destroy(stor);
    qn_json_destroy_object(ext.server_end.put_policy);
    qn_mac_destroy(mac);
    return 0;
}
