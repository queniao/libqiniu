#include <stdio.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string src_url;
    qn_string bucket;
    qn_string key;
    qn_string fetch_ret;
    qn_storage_ptr stor;
    qn_stor_auth auth;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 6) {
        printf("Usage: qfetch <ACCESS_KEY> <SECRET_KEY> <SRC_URL> <BUCKET> <KEY>\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    src_url = argv[3];
    bucket = argv[4];
    key = argv[5];

    stor = qn_stor_create();
    if (!stor) {
        printf("Cannot initialize a new storage object.\n");
        return 1;
    } // if

    memset(&auth, 0, sizeof(auth));
    auth.server_end.mac = mac;

    if (!qn_stor_fetch(stor, &auth, src_url, bucket, key, NULL)) {
        printf("Cannot fetch the `%s` to the `%s:%s` file.\n", src_url, bucket, key);
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) {
        printf("%s\n", qn_str_cstr(hdr_ent));
    } // while

    fetch_ret = qn_json_object_to_string(qn_stor_get_object_body(stor));
    if (!fetch_ret) {
        printf("Cannot format the object body form /stat interface.\n");
        return 3;
    } // if

    printf("%s\n", fetch_ret);
    qn_str_destroy(fetch_ret);

    qn_stor_destroy(stor);
    qn_mac_destroy(mac);
    return 0;
}
