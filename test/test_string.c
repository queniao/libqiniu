#include <CUnit/Basic.h>

#include "qiniu/base/string.h"

// ---- test creation & destruction ----

void test_duplicate(void)
{
    const char buf[] = {"This is a test line for create a new string by cloning raw string."};
    qn_string str = NULL;

    str = qn_cs_duplicate(buf);
    if (!str) {
        CU_FAIL("Cannot clone the a new string from raw string.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str), buf);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), buf);
    CU_ASSERT_EQUAL(qn_str_size(str), strlen(buf));

    qn_str_destroy(str);
} // test_duplicate

void test_join_list(void)
{
    const char buf1[] = {"AB"};
    const char buf2[] = {"CD"};
    const char buf3[] = {"EF"};
    qn_string srcs[3];
    qn_string str1 = NULL;
    qn_string str2 = NULL;
    qn_string str3 = NULL;
    
    srcs[0] = qn_cs_duplicate(buf1);
    if (!srcs[0]) {
        CU_FAIL("Cannot clone a raw string to source input #1.");
        return;
    } // if
    
    srcs[1] = qn_cs_duplicate(buf2);
    if (!srcs[1]) {
        CU_FAIL("Cannot clone a raw string to source input #2.");
        return;
    } // if
    
    srcs[2] = qn_cs_duplicate(buf3);
    if (!srcs[2]) {
        CU_FAIL("Cannot clone a raw string to source input #3.");
        return;
    } // if

    str1 = qn_str_join_list(",", srcs, 1);
    if (!str1) {
        CU_FAIL("Cannot join one string to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str1), qn_str_cstr(srcs[0]));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str1), "AB");
    CU_ASSERT_EQUAL(qn_str_size(str1), qn_str_size(srcs[0]));

    str2 = qn_str_join_list("::", srcs, 2);
    if (!str2) {
        CU_FAIL("Cannot join two strings to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str2), qn_str_cstr(srcs[0]));
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str2), qn_str_cstr(srcs[1]));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str2), "AB::CD");
    CU_ASSERT_EQUAL(qn_str_size(str2), qn_str_size(srcs[0]) + 2 + qn_str_size(srcs[1]));

    str3 = qn_str_join_list("", srcs, 3);
    if (!str3) {
        CU_FAIL("Cannot join three strings to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), qn_str_cstr(srcs[0]));
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), qn_str_cstr(srcs[1]));
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), qn_str_cstr(srcs[2]));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str3), "ABCDEF");
    CU_ASSERT_EQUAL(qn_str_size(str3), qn_str_size(srcs[0]) + qn_str_size(srcs[1]) + qn_str_size(srcs[2]));

    qn_str_destroy(str3);
    qn_str_destroy(str2);
    qn_str_destroy(str1);

    qn_str_destroy(srcs[2]);
    qn_str_destroy(srcs[1]);
    qn_str_destroy(srcs[0]);
} // test_join_list

void test_join(void)
{
    qn_string buf1 = "AB";
    qn_string buf2 = "CD";
    qn_string buf3 = "EF";
    qn_string str1 = NULL;
    qn_string str2 = NULL;
    qn_string str3 = NULL;
    
    str1 = qn_str_join(",", buf1, QN_STR_ARG_END);
    if (!str1) {
        CU_FAIL("Cannot join one string to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str1), buf1);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str1), "AB");
    CU_ASSERT_EQUAL(qn_str_size(str1), strlen(buf1));

    str2 = qn_str_join("::", buf1, buf2, QN_STR_ARG_END);
    if (!str2) {
        CU_FAIL("Cannot join two strings to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str2), buf1);
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str2), buf2);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str2), "AB::CD");
    CU_ASSERT_EQUAL(qn_str_size(str2), strlen(buf1) + 2 + strlen(buf2));

    str3 = qn_str_join("", buf1, buf2, buf3, QN_STR_ARG_END);
    if (!str3) {
        CU_FAIL("Cannot join three strings to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), buf1);
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), buf2);
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), buf3);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str3), "ABCDEF");
    CU_ASSERT_EQUAL(qn_str_size(str3), strlen(buf1) + strlen(buf2) + strlen(buf3));

    qn_str_destroy(str3);
    qn_str_destroy(str2);
    qn_str_destroy(str1);
} // test_join

void test_sprintf(void)
{
    qn_string str = NULL;
    char ret[] = {"Testing sprintf -123 ABCD."};
    
    str = qn_str_sprintf("Testing sprintf %d %s.", -123, "ABCD");
    if (!str) {
        CU_FAIL("Cannot sprintf a new string.");
        return;
    } // if

    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), ret);
    CU_ASSERT_EQUAL(qn_str_size(str), strlen(ret));

    qn_str_destroy(str);
} // test_sprintf

void test_snprintf(void)
{
    int len = 0;
    int str_len = 0;
    char ret[] = {"Testing sprintf -123 ABCD."};
    char buf[100] = {0};
    
    len = qn_str_snprintf(NULL, 0, "Testing sprintf %d %s.", -123, "ABCD");
    if (len < 0) {
        CU_FAIL("Cannot measure the length of formatting string.");
        return;
    } // if

    CU_ASSERT_EQUAL(len, strlen(ret));

    str_len = qn_str_snprintf(buf, sizeof(buf), "Testing sprintf %d %s.", -123, "ABCD");
    if (str_len < 0) {
        CU_FAIL("Cannot snprintf a new string.");
        return;
    } // if

    CU_ASSERT_STRING_EQUAL(buf, ret);
    CU_ASSERT_EQUAL(str_len, strlen(ret));
} // test_snprintf

CU_TestInfo test_normal_cases[] = {
    {"test_duplicate", test_duplicate},
    {"test_join_list", test_join_list},
    {"test_join", test_join},
    {"test_sprintf", test_sprintf},
    {"test_snprintf", test_snprintf},
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
