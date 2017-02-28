#include <stdio.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/base/errors.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    const char * bucket;
    const char * prefix = NULL;
    const char * delimiter = NULL;
    const char * limit = NULL;
    qn_string marker;
    qn_string list_ret_str;
    qn_json_object_ptr list_ret;
    qn_stor_list_extra_ptr le;
    qn_storage_ptr stor;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 4) {
        printf("Usage: qlist <ACCESS_KEY> <SECRET_KEY> <BUCKET> [PREFIX [DELIMITER [LIMIT]]]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    if (! mac) {
        printf("Cannot create a new mac due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    bucket = argv[3];
    if (argc > 4) prefix = argv[4];
    if (argc > 5) delimiter = argv[5];
    if (argc > 6) limit = argv[6];

    le = qn_stor_lse_create();
    if (! le) {
        qn_mac_destroy(mac);
        printf("Cannot create a copy extra due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    if (prefix) qn_stor_lse_set_prefix(le, prefix, delimiter);
    if (limit) qn_stor_lse_set_limit(le, atoi(limit));

    stor = qn_stor_create();
    if (! stor) {
        qn_stor_lse_destroy(le);
        qn_mac_destroy(mac);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    do {
        list_ret = qn_stor_ls_api_list(stor, mac, bucket, le);
        if (! list_ret) {
            qn_stor_destroy(stor);
            qn_stor_lse_destroy(le);
            qn_mac_destroy(mac);
            printf("Cannot list all files in the `%s` bucket due to application error `%s`.\n", bucket, qn_err_get_message());
            return 2;
        } // if

        hdr_itr = qn_stor_resp_get_header_iterator(stor);
        while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) printf("%s\n", qn_str_cstr(hdr_ent));
        qn_http_hdr_itr_destroy(hdr_itr);

        list_ret_str = qn_json_object_to_string(list_ret);
        if (! list_ret_str) {
            qn_stor_destroy(stor);
            qn_stor_lse_destroy(le);
            qn_mac_destroy(mac);
            printf("Cannot format the list result due to application error `%s`.\n", qn_err_get_message());
            return 3;
        } // if

        printf("%s\n", list_ret_str);
        qn_str_destroy(list_ret_str);

        marker = qn_json_get_string(list_ret, "marker", NULL);
        qn_stor_lse_set_marker(le, marker);
    } while (marker && strlen(marker));

    qn_stor_destroy(stor);
    qn_stor_lse_destroy(le);
    qn_mac_destroy(mac);

    return 0;
}
