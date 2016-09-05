#include <stdio.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string src_bucket = NULL;
    qn_string src_key = NULL;
    qn_string dest_bucket = NULL;
    qn_string dest_key = NULL;
    qn_storage_ptr stor = NULL;
    qn_stor_copy_extra ext;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 7) {
        printf("Usage: qcopy <ACCESS_KEY> <SECRET_KEY> <SRC_BUCKET> <SRC_KEY> <DEST_BUCKET> <DEST_BUCKET> [FORCE]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    src_bucket = argv[3];
    src_key = argv[4];
    dest_bucket = argv[5];
    dest_key = argv[6];

    stor = qn_stor_create();
    if (!stor) {
        printf("Cannot initialize a new storage object.\n");
        return 1;
    } // if

    memset(&ext, 0, sizeof(ext));
    ext.server_end.mac = mac;

    if (!qn_stor_copy(stor, src_bucket, src_key, dest_bucket, dest_key, &ext)) {
        printf("Cannot copy the `%s:%s` file to `%s:%s`.\n", src_bucket, src_key, dest_bucket, dest_bucket);
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) {
        printf("%s\n", qn_str_cstr(hdr_ent));
    } // while

    qn_stor_destroy(stor);
    qn_mac_destroy(mac);
    return 0;
}
