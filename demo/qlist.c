#include <stdio.h>
#include "qiniu/base/json_formatter.h"
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
    const char * prefix = NULL;
    const char * delimiter = NULL;
    qn_stor_list_extra ext;
    qn_storage_ptr stor;
    qn_stor_auth auth;

    if (argc < 4) {
        printf("Usage: qstat <ACCESS_KEY> <SECRET_KEY> <BUCKET> [PREFIX [DELIMITER]]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    bucket = argv[3];
    if (argc > 4) {
        prefix = argv[4];
    } // if
    if (argc > 5) {
        delimiter = argv[5];
    } // if

    stor = qn_stor_create();
    if (!stor) {
        printf("Cannot initialize a new storage object.\n");
        return 1;
    } // if

    memset(&auth, 0, sizeof(auth));
    auth.server_end.mac = mac;

    memset(&ext, 0, sizeof(ext));
    ext.item_processor = &n;
    ext.item_processor_callback = &item_processor_callback;
    ext.limit = 1000;
    ext.prefix = prefix;
    ext.delimiter = delimiter;

    if (!qn_stor_list(stor, &auth, bucket, &ext)) {
        printf("Cannot list all files in the `%s` bucket.\n", bucket);
        return 2;
    } // if

    qn_stor_destroy(stor);
    qn_mac_destroy(mac);
    return 0;
}
