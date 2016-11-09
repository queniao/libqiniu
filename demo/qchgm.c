#include <stdio.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/base/errors.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_stor_auth auth;
    qn_string bucket;
    qn_string key;
    qn_string mime;
    qn_string chgm_ret_str;
    qn_json_object_ptr chgm_ret;
    qn_storage_ptr stor;
    qn_stor_change_mime_extra ext;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 6) {
        printf("Demo qchgm - Change the mime type of the given file.\n");
        printf("Usage: qchgm <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <MIME>\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    bucket = argv[3];
    key = argv[4];
    mime = argv[5];

    stor = qn_stor_create();
    if (!stor) {
        qn_mac_destroy(mac);
        printf("Cannot initialize a new storage object.\n");
        return 1;
    } // if

    memset(&ext, 0, sizeof(ext));

    memset(&auth, 0, sizeof(auth));
    auth.server_end.mac = mac;

    chgm_ret = qn_stor_change_mime(stor, &auth, bucket, key, mime, &ext);
    qn_mac_destroy(mac);
    if (!chgm_ret) {
        qn_stor_destroy(stor);
        printf("Cannot change the mime type of the `%s:%s` file due to application error `%s`.\n", bucket, key, qn_err_get_message());
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) {
        printf("%s\n", qn_str_cstr(hdr_ent));
    } // while
    qn_http_hdr_itr_destroy(hdr_itr);

    chgm_ret_str = qn_json_object_to_string(chgm_ret);
    qn_stor_destroy(stor);

    if (!chgm_ret_str) {
        printf("Cannot format the change mime result object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    printf("%s\n", chgm_ret_str);
    qn_str_destroy(chgm_ret_str);

    return 0;
}
