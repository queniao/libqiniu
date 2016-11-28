#include <stdio.h>
#include <time.h>
#include "qiniu/base/errors.h"
#include "qiniu/base/json_formatter.h"
#include "qiniu/storage.h"

int main(int argc, char * argv[])
{
    const char * bucket;
    const char * key;
    const char * fname;
    qn_stor_rput_session_ptr ss = NULL;
    qn_mac_ptr mac;
    qn_string uptoken;
    qn_string put_ret_str;
    qn_json_object_ptr put_policy;
    qn_json_object_ptr put_ret;
    qn_storage_ptr stor;
    qn_stor_rput_extra ext;

    if (argc < 5) {
        printf("Demo qputf - Put single file in many HTTP sessions.\n");
        printf("Usage: qrputf <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <FNAME>\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    if (! mac) {
        printf("Cannot create a mac object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    bucket = argv[3];
    key = argv[4];
    fname = argv[5];

    memset(&ext, 0, sizeof(ext));
    ext.final_key = key;

    put_policy = qn_pp_create(bucket, key, time(NULL) + 3600);
    if (! put_policy) {
        qn_mac_destroy(mac);
        printf("Cannot create a put policy due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    uptoken = qn_pp_to_uptoken(put_policy, mac);
    qn_json_destroy_object(put_policy);
    qn_mac_destroy(mac);
    if (! uptoken) {
        printf("Cannot make a uptoken due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    stor = qn_stor_create();
    if (! stor) {
        qn_str_destroy(uptoken);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    put_ret = qn_stor_rp_put_file(stor, uptoken, &ss, fname, &ext);
    qn_str_destroy(uptoken);
    qn_stor_rs_destroy(ss);
    if (! put_ret) {
        qn_stor_destroy(stor);
        printf("Cannot put the file `%s` to `%s:%s` due to application error `%s`.\n", fname, bucket, key, qn_err_get_message());
        return 2;
    } // if

    put_ret_str = qn_json_object_to_string(put_ret);
    qn_stor_destroy(stor);
    if (! put_ret_str) {
        printf("Cannot format the result object due to application error `%s`.\n", qn_err_get_message());
        return 3;
    } // if

    printf("%s\n", put_ret_str);
    qn_str_destroy(put_ret_str);

    return 0;
}
