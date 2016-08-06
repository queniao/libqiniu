#include <CUnit/Basic.h>
#include "qiniu/http_header_parser.h"

// ---- Tests of nomral cases ----

void test_manipulating_headers(void)
{
    qn_bool ret = qn_false;
    qn_http_header_ptr hdr = NULL;
    const char * key = NULL;
    const char * val = NULL;
    qn_size val_size = 0;
    qn_string entry = NULL;

    hdr = qn_http_hdr_create();
    if (!hdr) {
        CU_FAIL("Cannot create a new HTTP header.");
        return;
    } // if

    key = "Content-Type";
    val = "test/json";
    ret = qn_http_hdr_set_raw(hdr, key, strlen(key), val, strlen(val));
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_http_hdr_size(hdr), 1);

    entry = qn_http_hdr_get_entry_raw(hdr, key, strlen(key));
    CU_ASSERT_NOT_EQUAL(qn_str_cstr(entry), NULL);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(entry), "Content-Type: test/json");

    ret = qn_http_hdr_get_raw(hdr, key, strlen(key), &val, &val_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(val_size, 9);
    CU_ASSERT_STRING_EQUAL(val, "test/json");

    qn_http_hdr_destroy(hdr);
}

void test_parsing_single_entry_with_single_value(void)
{
    qn_bool ret = qn_false;
    char buf[] = {"Content-Length: 386\r\n\r\n"};
    int buf_size = sizeof(buf);
    qn_string entry = NULL;
    const char * key = NULL;
    const char * val = NULL;
    qn_size val_size = 0;
    qn_http_hdr_parser_ptr prs = NULL;
    qn_http_header_ptr hdr = NULL;

    prs = qn_http_hdr_prs_create();
    if (!prs) {
        CU_FAIL("Cannot create a new HTTP header parser.");
        return;
    } // if

    ret = qn_http_hdr_prs_parse(prs, buf, &buf_size, &hdr);
    qn_http_hdr_prs_destroy(prs);

    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_http_hdr_size(hdr), 1);

    key = "Content-Length";
    entry = qn_http_hdr_get_entry_raw(hdr, key, strlen(key));
    CU_ASSERT_NOT_EQUAL(qn_str_cstr(entry), NULL);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(entry), "Content-Length: 386");

    ret = qn_http_hdr_get_raw(hdr, key, strlen(key), &val, &val_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(val_size, 3);
    CU_ASSERT_STRING_EQUAL(val, "386");

    qn_http_hdr_destroy(hdr);
}

CU_TestInfo test_normal_cases[] = {
    {"test_manipulating_headers", test_manipulating_headers},
    {"test_parsing_single_entry_with_single_value", test_parsing_single_entry_with_single_value},
    CU_TEST_INFO_NULL
};

CU_SuiteInfo suites[] = {
    {"test_normal_cases", NULL, NULL, test_normal_cases},
    CU_SUITE_INFO_NULL
};

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    } // if

    pSuite = CU_add_suite("Suite_Test_String", NULL, NULL);
    if (pSuite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    } // if

    if (CU_register_suites(suites) != CUE_SUCCESS) {
        printf("Cannot register test suites.\n");
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
} // main
