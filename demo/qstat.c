#include <stdio.h>
#include "qiniu/base/errors.h"
#include "qiniu/base/json_formatter.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string bucket;
    qn_string key;
    qn_string stat_ret_str;
    qn_json_object_ptr stat_ret;
    qn_storage_ptr stor;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 5) {
        printf("Usage: qstat <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY>\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    if (! mac) {
        printf("Cannot create a new mac due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    bucket = argv[3];
    key = argv[4];

    stor = qn_stor_create();
    if (!stor) {
        qn_mac_destroy(mac);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    stat_ret = qn_stor_mn_api_stat(stor, mac, bucket, key, NULL);
    qn_mac_destroy(mac);
    if (!stat_ret) {
        qn_stor_destroy(stor);
        printf("Cannot stat the `%s:%s` file due to application error `%s`.\n", bucket, key, qn_err_get_message());
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) printf("%s\n", qn_str_cstr(hdr_ent));
    qn_http_hdr_itr_destroy(hdr_itr);

    stat_ret_str = qn_json_object_to_string(stat_ret);
    qn_stor_destroy(stor);
    if (!stat_ret_str) {
        printf("Cannot format the result object due to application error `%s`.\n", qn_err_get_message());
        return 3;
    } // if

    printf("%s\n", stat_ret_str);
    qn_str_destroy(stat_ret_str);

    return 0;
}
