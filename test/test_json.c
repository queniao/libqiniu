#include <CUnit/Basic.h>

#include "base/json.h"

#define CU_ASSERT_LONG_DOUBLE_EQUAL(actual, expected, granularity) \
  { CU_assertImplementation(((fabsl((long double)(actual) - (expected)) <= fabsl((long double)(granularity)))), __LINE__, ("CU_ASSERT_LONG_DOUBLE_EQUAL(" #actual ","  #expected "," #granularity ")"), __FILE__, "", CU_FALSE); }

// ---- test parser ----

void test_parse_empty_object(void)
{
    qn_bool ret;
    const char buf[] = {"{}"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr obj = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &obj);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the empty object.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_object(obj));
    CU_ASSERT_TRUE(qn_json_is_empty(obj));

    qn_json_destroy(obj);
} // test_parse_empty_object

void test_parse_object_holding_one_element(void)
{
    qn_bool ret;
    const char buf[] = {"{\"trivial\":\"This is a trivial element.\"}"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr obj = NULL;
    qn_json_ptr elem = NULL;
    qn_string_ptr str = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &obj);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding one element.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_object(obj));
    CU_ASSERT_TRUE(!qn_json_is_empty(obj));

    elem = qn_json_get(obj, "trivial");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_string(elem));

    str = qn_json_to_string(elem);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), "This is a trivial element.") == 0);

    qn_json_destroy(obj);
} // test_parse_object_holding_one_element

void test_parse_object_holding_two_elements(void)
{
    qn_bool ret;
    const char buf[] = {"{\"trivial\":\"This is a trivial element.\",\"int\":-123}"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr obj = NULL;
    qn_json_ptr elem = NULL;
    qn_integer val = 0;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &obj);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding two elements.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_object(obj));
    CU_ASSERT_TRUE(!qn_json_is_empty(obj));

    elem = qn_json_get(obj, "int");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_integer(elem));

    val = qn_json_to_integer(elem);
    CU_ASSERT_TRUE(val == -123);

    qn_json_destroy(obj);
} // test_parse_object_holding_two_elements

void test_parse_object_holding_ordinary_elements(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_num\":+123.456,\"_true\":true,\"_false\":false,\"_null\":null}"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr obj = NULL;
    qn_json_ptr elem = NULL;
    qn_number num_val = 0.0L;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &obj);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding ordinary elements.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_object(obj));
    CU_ASSERT_TRUE(!qn_json_is_empty(obj));

    elem = qn_json_get(obj, "_null");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_null(elem));

    elem = qn_json_get(obj, "_false");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_boolean(elem));
    CU_ASSERT_TRUE(qn_json_to_boolean(elem) == false);

    elem = qn_json_get(obj, "_true");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_boolean(elem));
    CU_ASSERT_TRUE(qn_json_to_boolean(elem) == true);

    elem = qn_json_get(obj, "_num");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_number(elem));

    num_val = qn_json_to_number(elem);

    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 123.456L, 0.001L);

    qn_json_destroy(obj);
} // test_parse_object_holding_ordinary_elements

CU_TestInfo test_normal_cases_of_json_parsing[] = {
    {"test_parse_empty_object()", test_parse_empty_object},
    {"test_parse_object_holding_one_element()", test_parse_object_holding_one_element},
    {"test_parse_object_holding_two_elements()", test_parse_object_holding_two_elements},
    {"test_parse_object_holding_ordinary_elements", test_parse_object_holding_ordinary_elements},
    CU_TEST_INFO_NULL
};

CU_SuiteInfo suites[] = {
    {"test_normal_cases_of_json_parsing", NULL, NULL, NULL, NULL, test_normal_cases_of_json_parsing},
    CU_SUITE_INFO_NULL
};

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    } // if

    pSuite = CU_add_suite("Suite_Test_JSON", NULL, NULL);
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
