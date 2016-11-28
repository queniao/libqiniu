#include <stdio.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/base/errors.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string src_url;
    qn_string bucket;
    qn_string key;
    qn_string fetch_ret_str;
    qn_json_object_ptr fetch_ret;
    qn_storage_ptr stor;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 6) {
        printf("Demo qfetch - Fetch a file from any other server as if it is uploaded.\n");
        printf("Usage: qfetch <ACCESS_KEY> <SECRET_KEY> <SRC_URL> <BUCKET> <KEY>\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    if (! mac) {
        printf("Cannot create a new mac due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    src_url = argv[3];
    bucket = argv[4];
    key = argv[5];

    stor = qn_stor_create();
    if (! stor) {
        qn_mac_destroy(mac);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    fetch_ret = qn_stor_fetch(stor, mac, src_url, bucket, key, NULL);
    qn_mac_destroy(mac);
    if (! fetch_ret) {
        qn_stor_destroy(stor);
        printf("Cannot fetch the `%s` to the `%s:%s` file due to application error `%s`.\n", src_url, bucket, key, qn_err_get_message());
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) printf("%s\n", qn_str_cstr(hdr_ent));
    qn_http_hdr_itr_destroy(hdr_itr);

    fetch_ret_str = qn_json_object_to_string(fetch_ret);
    qn_stor_destroy(stor);
    if (!fetch_ret_str) {
        printf("Cannot format the object body returned from the fetch interface due to application error `%s`.\n", qn_err_get_message());
        return 3;
    } // if

    printf("%s\n", fetch_ret_str);
    qn_str_destroy(fetch_ret_str);

    return 0;
}
