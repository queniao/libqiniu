#include <stdio.h>
#include "qiniu/base/json_formatter.h"
#include "qiniu/base/errors.h"
#include "qiniu/storage.h"

qn_bool item_processor_callback(void * user_data, qn_json_object_ptr item)
{
    int * i = (int *) user_data;
    qn_string str;

    if (!item) {
        fprintf(stderr, "No invalid item.\n");
        return qn_false;
    } // if
    
    str = qn_json_object_to_string(item);
    if (!str) {
        fprintf(stderr, "Cannot format object item.\n");
        return qn_false;
    } // if
    printf("%d %s\n", ++*i, str);
    qn_str_destroy(str);
    return qn_true;
}

int main(int argc, char * argv[])
{
    int n = 0;
    qn_mac_ptr mac;
    const char * bucket;
    const char * in_list = NULL;
    const char * prefix = NULL;
    const char * delimiter = NULL;
    qn_string list_ret_str;
    qn_json_object_ptr list_ret;
    qn_stor_list_extra ext;
    qn_storage_ptr stor;
    qn_stor_auth auth;
    qn_http_hdr_iterator_ptr hdr_itr;
    qn_string hdr_ent;

    if (argc < 4) {
        printf("Usage: qlist <ACCESS_KEY> <SECRET_KEY> <BUCKET> [IN_LIST [PREFIX [DELIMITER]]]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    bucket = argv[3];
    if (argc > 4) in_list = argv[4];
    if (argc > 5) prefix = argv[5];
    if (argc > 6) delimiter = argv[6];

    stor = qn_stor_create();
    if (!stor) {
        printf("Cannot initialize a new storage object.\n");
        return 1;
    } // if

    memset(&auth, 0, sizeof(auth));
    auth.server_end.mac = mac;

    memset(&ext, 0, sizeof(ext));
    if (in_list && strlen(in_list) > 0) {
        ext.item_processor = &n;
        ext.item_processor_callback = &item_processor_callback;
    } else {
        in_list = NULL;
    } // if

    ext.limit = 1000;

    if (prefix && strlen(prefix) > 0) {
        ext.prefix = prefix;
    } // if

    ext.delimiter = delimiter;

    list_ret = qn_stor_list(stor, &auth, bucket, &ext);
    qn_mac_destroy(mac);
    if (!list_ret) {
        qn_stor_destroy(stor);
        printf("Cannot list all files in the `%s` bucket due to application error `%s`.\n", bucket, qn_err_get_message());
        return 2;
    } // if

    hdr_itr = qn_stor_resp_get_header_iterator(stor);
    while ((hdr_ent = qn_http_hdr_itr_next_entry(hdr_itr))) {
        printf("%s\n", qn_str_cstr(hdr_ent));
    } // while
    qn_http_hdr_itr_destroy(hdr_itr);

    if (!in_list) {
        list_ret_str = qn_json_object_to_string(list_ret);
        if (!list_ret_str) {
            qn_stor_destroy(stor);
            printf("Cannot format the list result due to application error `%s`.\n", qn_err_get_message());
            return 3;
        } // if

        printf("%s\n", list_ret_str);
        qn_str_destroy(list_ret_str);
    } // if
    qn_stor_destroy(stor);

    return 0;
}
