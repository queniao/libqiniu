#include <CUnit/Basic.h>

#include "base/string.h"

// ---- test creation & destruction ----

void test_create(void)
{
    const char buf[] = {"This is a test line for create a string."};
    qn_size buf_len = strlen(buf);
    qn_string_ptr str = NULL;

    str = qn_str_create(buf, buf_len);
    if (!str) {
        CU_FAIL("Cannot parse the a new string.");
        return;
    } // if

    CU_ASSERT_PTR_NOT_EQUAL(qn_str_cstr(str), buf);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), buf);
    CU_ASSERT_EQUAL(qn_str_size(str), buf_len);

    qn_str_destroy(str);
} // test_create

CU_TestInfo test_normal_cases[] = {
    {"test_create", test_create},
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
