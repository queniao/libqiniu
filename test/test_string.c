#include <CUnit/Basic.h>

#include "base/string.h"

// ---- test creation & destruction ----

void test_create(void)
{
    const char buf[] = {"This is a test line for create a new string."};
    qn_size buf_len = strlen(buf);
    qn_string_ptr str = NULL;

    str = qn_str_create(buf, buf_len);
    if (!str) {
        CU_FAIL("Cannot create the a new string.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str), buf);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), buf);
    CU_ASSERT_EQUAL(qn_str_size(str), buf_len);

    qn_str_destroy(str);
} // test_create

void test_clone_raw(void)
{
    const char buf[] = {"This is a test line for create a new string by cloning raw string."};
    qn_string_ptr str = NULL;

    str = qn_str_clone_raw(buf);
    if (!str) {
        CU_FAIL("Cannot clone the a new string from raw string.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str), buf);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), buf);
    CU_ASSERT_EQUAL(qn_str_size(str), strlen(buf));

    qn_str_destroy(str);
} // test_clone_raw

void test_duplicate(void)
{
    const char buf[] = {"This is a test line for create a new string."};
    qn_size buf_len = strlen(buf);
    qn_string_ptr str_ori = NULL;
    qn_string_ptr str_dup = NULL;

    str_ori = qn_str_create(buf, buf_len);
    if (!str_ori) {
        CU_FAIL("Cannot create the a new origin string.");
        return;
    } // if

    str_dup = qn_str_duplicate(str_ori);
    if (!str_dup) {
        CU_FAIL("Cannot create the a new duplicated string.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str_dup), qn_str_cstr(str_ori));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str_dup), buf);
    CU_ASSERT_EQUAL(qn_str_size(str_dup), buf_len);

    qn_str_destroy(str_dup);
    qn_str_destroy(str_ori);
} // test_duplicate

void test_join_raw(void)
{
    const char buf1[] = {"AB"};
    const char buf2[] = {"CD"};
    const char buf3[] = {"EF"};
    qn_string_ptr str2 = NULL;
    qn_string_ptr str3 = NULL;

    str2 = qn_str_join_raw("::", buf1, strlen(buf1), buf2, strlen(buf2), QN_STR_ARG_END);
    if (!str2) {
        CU_FAIL("Cannot join two raw strings to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str2), buf1);
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str2), buf2);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str2), "AB::CD");
    CU_ASSERT_EQUAL(qn_str_size(str2), strlen(buf1) + 2 + strlen(buf2));

    str3 = qn_str_join_raw("", buf1, strlen(buf1), buf2, strlen(buf2), buf3, strlen(buf3), QN_STR_ARG_END);
    if (!str3) {
        CU_FAIL("Cannot join three raw strings to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), buf1);
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), buf2);
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), buf3);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str3), "ABCDEF");
    CU_ASSERT_EQUAL(qn_str_size(str3), strlen(buf1) + strlen(buf2) + strlen(buf3));

    qn_str_destroy(str3);
    qn_str_destroy(str2);
} // test_join_raw

void test_join(void)
{
    const char buf1[] = {"AB"};
    const char buf2[] = {"CD"};
    const char buf3[] = {"EF"};
    qn_string_ptr srcs[3];
    qn_string_ptr str1 = NULL;
    qn_string_ptr str2 = NULL;
    qn_string_ptr str3 = NULL;
    
    srcs[0] = qn_str_clone_raw(buf1);
    if (!srcs[0]) {
        CU_FAIL("Cannot clone a raw string to source input #1.");
        return;
    } // if
    
    srcs[1] = qn_str_clone_raw(buf2);
    if (!srcs[1]) {
        CU_FAIL("Cannot clone a raw string to source input #2.");
        return;
    } // if
    
    srcs[2] = qn_str_clone_raw(buf3);
    if (!srcs[2]) {
        CU_FAIL("Cannot clone a raw string to source input #3.");
        return;
    } // if

    str1 = qn_str_join(",", srcs, 1);
    if (!str1) {
        CU_FAIL("Cannot join one string to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str1), qn_str_cstr(srcs[0]));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str1), "AB");
    CU_ASSERT_EQUAL(qn_str_size(str1), qn_str_size(srcs[0]));

    str2 = qn_str_join("::", srcs, 2);
    if (!str2) {
        CU_FAIL("Cannot join two strings to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str2), qn_str_cstr(srcs[0]));
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str2), qn_str_cstr(srcs[1]));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str2), "AB::CD");
    CU_ASSERT_EQUAL(qn_str_size(str2), qn_str_size(srcs[0]) + 2 + qn_str_size(srcs[1]));

    str3 = qn_str_join("", srcs, 3);
    if (!str3) {
        CU_FAIL("Cannot join three strings to a new one.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), qn_str_cstr(srcs[0]));
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), qn_str_cstr(srcs[1]));
    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str3), qn_str_cstr(srcs[2]));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str3), "ABCDEF");
    CU_ASSERT_EQUAL(qn_str_size(str2), qn_str_size(srcs[0]) + qn_str_size(srcs[1]) + qn_str_size(srcs[2]));

    qn_str_destroy(str3);
    qn_str_destroy(str2);
    qn_str_destroy(str1);

    qn_str_destroy(srcs[2]);
    qn_str_destroy(srcs[1]);
    qn_str_destroy(srcs[0]);
} // test_join

CU_TestInfo test_normal_cases[] = {
    {"test_create", test_create},
    {"test_clone_raw", test_clone_raw},
    {"test_duplicate", test_duplicate},
    {"test_join_raw", test_join_raw},
    {"test_join", test_join},
    CU_TEST_INFO_NULL
};

CU_SuiteInfo suites[] = {
    {"test_normal_cases", NULL, NULL, NULL, NULL, test_normal_cases},
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
