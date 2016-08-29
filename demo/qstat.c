#include <stdio.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string bucket = NULL;
    qn_string key = NULL;
    qn_string stat_ret = NULL;
    qn_storage_ptr stor = NULL;
    qn_stor_query_extra ext;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 4) {
        printf("Usage: qdnurl <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY>\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    bucket = argv[3];
    key = argv[4];

    stor = qn_stor_create();
    if (!stor) {
        printf("Cannot initialize a new storage object.\n");
        return 1;
    } // if

    memset(&ext, 0, sizeof(ext));
    ext.mac = mac;

    if (!qn_stor_stat(stor, bucket, key, &ext)) {
        printf("Cannot stat the `%s:%s` file.\n", bucket, key);
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) {
        printf("%s\n", qn_str_cstr(hdr_ent));
    } // while

    stat_ret = qn_json_object_to_string(qn_stor_get_object_body(stor));
    if (!stat_ret) {
        printf("Cannot format the object body form /stat interface.\n");
        return 3;
    } // if

    printf("%s\n", stat_ret);
    qn_str_destroy(stat_ret);

    qn_stor_destroy(stor);
    qn_mac_destroy(mac);
    return 0;
}
