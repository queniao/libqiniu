#include <stdio.h>
#include <time.h>
#include "qiniu/base/errors.h"
#include "qiniu/base/json_formatter.h"
#include "qiniu/storage.h"
#include "qiniu/easy.h"

int main(int argc, char * argv[])
{
    int i = 0;
    const char * bucket;
    const char * key;
    const char * fname;
    const char * return_url = NULL;
    const char * return_body = NULL;
    const char * pos = NULL;
    qn_mac_ptr mac;
    qn_string local_qetag;
    qn_string uptoken;
    qn_string put_ret_str;
    qn_json_object_ptr put_policy;
    qn_json_object_ptr put_ret;
    qn_ud_variable_ptr ud_vars = NULL;
    qn_easy_ptr easy;
    qn_easy_put_extra_ptr pe;

    if (argc < 6) {
        printf("Demo qeputf - Put single file in an easy and smart way.\n");
        printf("Usage: qeputf <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <FNAME> [PUT_POLICY_FIELD=\"PUT_POLICY_VALUE\"]\n");
        return 0;
    } // if

    mac = qn_mac_create(argv[1], argv[2]);
    if (! mac) {
        printf("Cannot create a new mac due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    bucket = argv[3];
    key = argv[4];
    fname = argv[5];

    put_policy = qn_stor_pp_create(bucket, key, time(NULL) + 3600);
    if (! put_policy) {
        qn_mac_destroy(mac);
        printf("Cannot create a put policy due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    if (argc > 6) {
        for (i = 6; i < argc; i += 1) {
            if (strncmp(argv[i], "x:", 2) == 0) {
                pos = strchr(argv[i], '=');
                if (! pos) {
                    printf("Unknown option: [%s], skipped.\n", argv[i]);
                    continue;
                } // if

                if (! ud_vars && ! (ud_vars = qn_ud_var_create())) {
                    printf("Cannot create a user-defined variable table due to application error `%s`\n", qn_err_get_message());
                    qn_json_destroy_object(put_policy);
                    exit(1);
                } // if

                if (! qn_ud_var_set_raw(ud_vars, argv[i], pos - argv[i], pos + 1, strlen(pos + 1))) {
                    printf("Cannot set a user-defined variable table due to application error `%s`\n", qn_err_get_message());
                    qn_json_destroy_object(put_policy);
                    exit(1);
                } // if
            } else {
                pos = strchr(argv[i], '=');
                if (! pos) {
                    printf("Unknown option: [%s], skipped.\n", argv[i]);
                    continue;
                } // if

                if (strncmp(argv[i], "RETURN_URL", 10) == 0) {
                    return_url = pos + 1;
                } else if (strncmp(argv[i], "RETURN_BODY", 11) == 0) {
                    return_body = pos + 1;
                } // if
            } // if
        } // for

        if (return_url) {
            if (! qn_stor_pp_return_to_server(put_policy, return_url, return_body)) {
                printf("Cannot set return url and body due to application error `%s`\n", qn_err_get_message());
                qn_json_destroy_object(put_policy);
                exit(1);
            } // if
        } else if (return_body) {
            if (! qn_stor_pp_return_to_client(put_policy, return_body)) {
                printf("Cannot set return url and body due to application error `%s`\n", qn_err_get_message());
                qn_json_destroy_object(put_policy);
                exit(1);
            } // if
        } // if
    } // if

    uptoken = qn_stor_pp_to_uptoken(put_policy, mac);
    qn_json_destroy_object(put_policy);
    qn_mac_destroy(mac);
    if (! uptoken) {
        printf("Cannot make an uptoken due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    pe = qn_easy_pe_create();
    if (! pe) {
        qn_str_destroy(uptoken);
        printf("Cannot create a put extra due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    qn_easy_pe_set_final_key(pe, key);
    qn_easy_pe_set_qetag_checking(pe, qn_true);

    qn_easy_pe_set_user_defined_variables(pe, ud_vars);

    easy = qn_easy_create();
    if (! easy) {
        qn_easy_pe_destroy(pe);
        qn_str_destroy(uptoken);
        printf("Cannot initialize a new storage object due to application error `%s`.\n", qn_err_get_message());
        return 1;
    } // if

    put_ret = qn_easy_put_file(easy, uptoken, fname, pe);
    if ((local_qetag = qn_easy_pe_get_qetag(pe))) printf("local qetag: %s\n", qn_str_cstr(local_qetag));

    qn_easy_pe_destroy(pe);
    qn_str_destroy(uptoken);

    if (! put_ret) {
        qn_easy_destroy(easy);
        printf("Cannot put the file `%s` to `%s:%s` due to application error `%s`.\n", fname, bucket, key, qn_err_get_message());
        return 2;
    } // if

    put_ret_str = qn_json_object_to_string(put_ret);
    qn_easy_destroy(easy);
    if (! put_ret_str) {
        printf("Cannot format the result object due to application error `%s`.\n", qn_err_get_message());
        return 3;
    } // if

    printf("%s\n", put_ret_str);
    qn_str_destroy(put_ret_str);

    return 0;
}
