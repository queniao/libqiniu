#include <CUnit/Basic.h>

#include "qiniu/base/errors.h"
#include "qiniu/base/json.h"
#include "qiniu/base/json_parser.h"
#include "qiniu/base/json_formatter.h"

#define CU_ASSERT_LONG_DOUBLE_EQUAL(actual, expected, granularity) \
  { CU_assertImplementation(((fabsl((long double)(actual) - (expected)) <= fabsl((long double)(granularity)))), __LINE__, ("CU_ASSERT_LONG_DOUBLE_EQUAL(" #actual ","  #expected "," #granularity ")"), __FILE__, "", CU_FALSE); }

// ---- test functions ----

void test_manipulate_object(void)
{
    qn_bool ret = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_string str = NULL;
    char buf[] = {"A line for creating string element."};
    size_t buf_len = strlen(buf);

    obj_root = qn_json_create_object();
    CU_ASSERT_FATAL(obj_root != NULL);

    // set a string element
    ret = qn_json_set_text(obj_root, "_str", buf, buf_len);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 1);

    // set a number element
    ret = qn_json_set_number(obj_root, "_num", -9.99L);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 2);

    // set a integer element
    ret = qn_json_set_integer(obj_root, "_int", 256);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 3);

    // set a null element
    ret = qn_json_set_null(obj_root, "_null");
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 4);

    // set a boolean element
    ret = qn_json_set_boolean(obj_root, "_false", qn_false);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 5);

    // check the null element
    qn_json_unset(obj_root, "_null");
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 4);

    // check the integer element
    CU_ASSERT_EQUAL(qn_json_get_integer(obj_root, "_int", 0), 256);

    qn_json_unset(obj_root, "_int");
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 3);

    // check the number element
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_get_number(obj_root, "_num", 0.0L), -9.99L, 0.01L);
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 3);

    // check the string element
    str = qn_json_get_string(obj_root, "_str", NULL);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), buf) == 0);
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 3);

    qn_json_unset(obj_root, "_false");
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 2);

    qn_json_unset(obj_root, "_str");
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 1);

    qn_json_unset(obj_root, "_num");
    CU_ASSERT_EQUAL(qn_json_size_object(obj_root), 0);

    qn_json_destroy_object(obj_root);
}

void test_manipulate_array(void)
{
    qn_bool ret = qn_false;
    qn_json_array_ptr arr_root = NULL;
    qn_string str = NULL;
    char buf[] = {"A line for creating string element."};
    size_t buf_len = strlen(buf);

    arr_root = qn_json_create_array();
    CU_ASSERT_FATAL(arr_root != NULL);

    // unshift a string element
    ret = qn_json_unshift_text(arr_root, buf, buf_len);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 1);

    // push a number element
    ret = qn_json_push_number(arr_root, -9.99L);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 2);

    // push a integer element
    ret = qn_json_push_integer(arr_root, 256);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 3);

    // unshift a null element
    ret = qn_json_unshift_null(arr_root);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 4);

    // push a boolean element
    ret = qn_json_push_boolean(arr_root, qn_false);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 5);

    // check the first element (null)
    qn_json_shift(arr_root);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 4);

    // check the second element (string)
    str = qn_json_pick_string(arr_root, 0, NULL);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), buf) == 0);

    qn_json_shift(arr_root);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 3);

    // check the last element (boolean == false)
    CU_ASSERT_TRUE(qn_json_pick_boolean(arr_root, 2, qn_true) == qn_false);
    qn_json_pop(arr_root);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 2);

    // check the last element (int == 256)
    CU_ASSERT_TRUE(qn_json_pick_integer(arr_root, 1, 0) != 0);
    CU_ASSERT_TRUE(qn_json_pick_integer(arr_root, 1, 0) == 256);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 2);

    // check the first element
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_pick_number(arr_root, 0, 0.0L), -9.99L, 0.01L);
    CU_ASSERT_EQUAL(qn_json_size_array(arr_root), 2);

    qn_json_destroy_array(arr_root);
}

CU_TestInfo test_normal_cases_of_json_manipulating[] = {
    {"test_manipulate_object()", test_manipulate_object},
    {"test_manipulate_array()", test_manipulate_array},
    CU_TEST_INFO_NULL
};

// ---- normal test case of parsing ----

void test_parse_empty_object(void)
{
    qn_bool ret;
    const char buf[] = {"{}"};
    size_t buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the empty object.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_empty_object(obj_root));

    qn_json_destroy_object(obj_root);
}

void test_parse_object_holding_one_element(void)
{
    qn_bool ret;
    const char buf[] = {"{\"trivial\":\"This is a trivial element.\"}"};
    size_t buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_string str = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding one element.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_object(obj_root));

    str = qn_json_get_string(obj_root, "trivial", NULL);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), "This is a trivial element.") == 0);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_holding_two_elements(void)
{
    qn_bool ret;
    const char buf[] = {"{\"trivial\":\"This is a trivial element.\",\"int\":-123}"};
    size_t buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding two elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_object(obj_root));

    CU_ASSERT_TRUE(qn_json_get_integer(obj_root, "int", 0) == -123);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_holding_ordinary_elements(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_num\":+123.456,\"_true\":true,\"_false\":false,\"_null\":null}"};
    size_t buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding ordinary elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_object(obj_root));

    CU_ASSERT_TRUE(qn_json_get_boolean(obj_root, "_false", qn_true) == qn_false);
    CU_ASSERT_TRUE(qn_json_get_boolean(obj_root, "_true", qn_false) == qn_true);
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_get_number(obj_root, "_num", 0.0L), 123.456L, 0.001L);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_holding_empty_complex_elements(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_arr\":[],\"_obj\":{}}"};
    size_t buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding empty complex elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_object(obj_root));

    arr_elem = qn_json_get_array(obj_root, "_arr", NULL);

    CU_ASSERT_TRUE(arr_elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_empty_array(arr_elem));

    obj_elem = qn_json_get_object(obj_root, "_obj", NULL);

    CU_ASSERT_TRUE(obj_elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_empty_object(obj_elem));

    qn_json_destroy_object(obj_root);
}

void test_parse_object_holding_embedded_objects(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_obj\":{\"_num\":+123.456,\"_true\":true,\"_false\":false,\"_null\":null},\"_obj2\":{}}"};
    size_t buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding embedded objects.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_object(obj_root));

    obj_elem = qn_json_get_object(obj_root, "_obj", NULL);

    CU_ASSERT_TRUE(obj_elem != NULL);
    CU_ASSERT_TRUE(!qn_json_is_empty_object(obj_elem));

    CU_ASSERT_TRUE(qn_json_get_boolean(obj_elem, "_false", qn_true) == qn_false);
    CU_ASSERT_TRUE(qn_json_get_boolean(obj_elem, "_true", qn_false) == qn_true);

    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_get_number(obj_elem, "_num", 0.0L), 123.456L, 0.001L);

    obj_elem = qn_json_get_object(obj_root, "_obj2", NULL);

    CU_ASSERT_TRUE(obj_elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_empty_object(obj_elem));

    qn_json_destroy_object(obj_root);
}

void test_parse_object_holding_utf8_string(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_str\":\"工人\"}"};
    size_t buf_len = strlen(buf);
    qn_string str;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    str = qn_json_get_string(obj_root, "_str", NULL);
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "工人");

    qn_json_destroy_object(obj_root);
}

void test_parse_object_integer_value_in_next_chunk_followed_by_others(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_int\":"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"12345, \"_str\":\"ABC\"}"};
    size_t buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_integer(obj_root, "_int", -1);
    CU_ASSERT_EQUAL(val, 12345);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_key_input_in_two_chunks_1(void)
{
    qn_bool ret;
    const char buf[] = {"{\""};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"_int\":345}"};
    size_t buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_integer(obj_root, "_int", -1);
    CU_ASSERT_EQUAL(val, 345);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_key_input_in_two_chunks_2(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_i"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"nt\":678}"};
    size_t buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_integer(obj_root, "_int", -1);
    CU_ASSERT_EQUAL(val, 678);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_key_input_in_two_chunks_3(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_int"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"\":90}"};
    size_t buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_integer(obj_root, "_int", -1);
    CU_ASSERT_EQUAL(val, 90);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_key_input_in_two_chunks_4(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_int\\"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"\"\":555}"};
    size_t buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_integer(obj_root, "_int\"", -1);
    CU_ASSERT_EQUAL(val, 555);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_string_input_in_two_chunks_1(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_str\":\""};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"农民\"}"};
    size_t buf2_len = strlen(buf2);
    qn_json_string str;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    str = qn_json_get_string(obj_root, "_str", NULL);
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "农民");

    qn_json_destroy_object(obj_root);
}

void test_parse_object_string_input_in_two_chunks_2(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_str\":\"学"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"生\"}"};
    size_t buf2_len = strlen(buf2);
    qn_json_string str;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    str = qn_json_get_string(obj_root, "_str", NULL);
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "学生");

    qn_json_destroy_object(obj_root);
}

void test_parse_object_string_input_in_two_chunks_3(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_str\":\"医生"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"\"}"};
    size_t buf2_len = strlen(buf2);
    qn_json_string str;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    str = qn_json_get_string(obj_root, "_str", NULL);
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "医生");

    qn_json_destroy_object(obj_root);
}

void test_parse_object_string_input_in_two_chunks_4(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_str\":\"工程师\\"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"\"\"}"};
    size_t buf2_len = strlen(buf2);
    qn_json_string str;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    str = qn_json_get_string(obj_root, "_str", NULL);
    CU_ASSERT_PTR_NOT_NULL(str);
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(str), "工程师\"");

    qn_json_destroy_object(obj_root);
}

void test_parse_object_integer_value_input_in_two_chunks(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_int\":12"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"345}"};
    size_t buf2_len = strlen(buf2);
    qn_json_integer val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_integer(obj_root, "_int", -1);
    CU_ASSERT_EQUAL(val, 12345);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_true_value_input_in_two_chunks_1(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_true\":t"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"rue}"};
    size_t buf2_len = strlen(buf2);
    qn_json_boolean val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_boolean(obj_root, "_true", qn_false);
    CU_ASSERT_TRUE(val);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_true_value_input_in_two_chunks_2(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_true\":tr"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"ue}"};
    size_t buf2_len = strlen(buf2);
    qn_json_boolean val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_boolean(obj_root, "_true", qn_false);
    CU_ASSERT_TRUE(val);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_true_value_input_in_two_chunks_3(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_true\":tru"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"e}"};
    size_t buf2_len = strlen(buf2);
    qn_json_boolean val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_boolean(obj_root, "_true", qn_false);
    CU_ASSERT_TRUE(val);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_false_value_input_in_two_chunks_1(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_false\":f"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"alse}"};
    size_t buf2_len = strlen(buf2);
    qn_json_boolean val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_boolean(obj_root, "_false", qn_true);
    CU_ASSERT_FALSE(val);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_false_value_input_in_two_chunks_2(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_false\":fal"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"se}"};
    size_t buf2_len = strlen(buf2);
    qn_json_boolean val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_boolean(obj_root, "_false", qn_true);
    CU_ASSERT_FALSE(val);

    qn_json_destroy_object(obj_root);
}

void test_parse_object_false_value_input_in_two_chunks_3(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_false\":fals"};
    size_t buf_len = strlen(buf);
    const char buf2[] = {"e}"};
    size_t buf2_len = strlen(buf2);
    qn_json_boolean val;
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    CU_ASSERT_FALSE(ret);
    CU_ASSERT_TRUE(qn_err_json_is_need_more_text_input());

    ret = qn_json_prs_parse_object(prs, buf2, &buf2_len, &obj_root);
    CU_ASSERT_TRUE(ret);

    val = qn_json_get_boolean(obj_root, "_false", qn_true);
    CU_ASSERT_FALSE(val);

    qn_json_destroy_object(obj_root);
}

// ----

void test_parse_empty_array(void)
{
    qn_bool ret;
    const char buf[] = {"[]"};
    size_t buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the empty array.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_empty_array(arr_root));

    qn_json_destroy_array(arr_root);
}

void test_parse_array_holding_one_element(void)
{
    qn_bool ret;
    const char buf[] = {"[\"This is a trivial element.\"]"};
    size_t buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_string str = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding one element.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_array(arr_root));

    str = qn_json_pick_string(arr_root, 0, NULL);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), "This is a trivial element.") == 0);

    qn_json_destroy_array(arr_root);
}

void test_parse_array_holding_two_elements(void)
{
    qn_bool ret;
    const char buf[] = {"[\"This is a trivial element.\",-123]"};
    size_t buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding two elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_array(arr_root));
    CU_ASSERT_TRUE(qn_json_pick_integer(arr_root, 1, 0) == -123);

    qn_json_destroy_array(arr_root);
}

void test_parse_array_holding_ordinary_elements(void)
{
    qn_bool ret;
    const char buf[] = {"[+123.456,true,false,null]"};
    size_t buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding ordinary elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_array(arr_root));

    CU_ASSERT_TRUE(qn_json_pick_boolean(arr_root, 2, qn_true) == qn_false);
    CU_ASSERT_TRUE(qn_json_pick_boolean(arr_root, 1, qn_false) == qn_true);
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_pick_number(arr_root, 0, 0.0L), 123.456L, 0.001L);

    qn_json_destroy_array(arr_root);
}

void test_parse_array_holding_empty_complex_elements(void)
{
    qn_bool ret;
    const char buf[] = {"[{},[]]"};
    size_t buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding empty complex elements.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_array(arr_root));

    obj_elem = qn_json_pick_object(arr_root, 0, NULL);

    CU_ASSERT_TRUE(obj_elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_empty_object(obj_elem));

    arr_elem = qn_json_pick_array(arr_root, 1, NULL);

    CU_ASSERT_TRUE(arr_elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_empty_array(arr_elem));

    qn_json_destroy_array(arr_root);
}

void test_parse_array_holding_embedded_arrays(void)
{
    qn_bool ret;
    const char buf[] = {"[[+123.456,true,false,null],[]]"};
    size_t buf_len = strlen(buf);
    qn_json_array_ptr arr_root = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_array(prs, buf, &buf_len, &arr_root);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding embedded arrays.");
        return;
    } // if

    CU_ASSERT_TRUE(!qn_json_is_empty_array(arr_root));

    arr_elem = qn_json_pick_array(arr_root, 0, NULL);

    CU_ASSERT_TRUE(arr_elem != NULL);
    CU_ASSERT_TRUE(!qn_json_is_empty_array(arr_elem));

    CU_ASSERT_TRUE(qn_json_pick_boolean(arr_elem, 2, qn_true) == qn_false);
    CU_ASSERT_TRUE(qn_json_pick_boolean(arr_elem, 1, qn_false) == qn_true);
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_pick_number(arr_elem, 0, 0.0L), 123.456L, 0.001L);

    arr_elem = qn_json_pick_array(arr_root, 1, NULL);

    CU_ASSERT_TRUE(arr_elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_empty_array(arr_elem));

    qn_json_destroy_array(arr_root);
}

CU_TestInfo test_normal_cases_of_json_parsing[] = {
    {"test_parse_empty_object()", test_parse_empty_object},
    {"test_parse_object_holding_one_element()", test_parse_object_holding_one_element},
    {"test_parse_object_holding_two_elements()", test_parse_object_holding_two_elements},
    {"test_parse_object_holding_ordinary_elements()", test_parse_object_holding_ordinary_elements},
    {"test_parse_object_holding_empty_complex_elements()", test_parse_object_holding_empty_complex_elements},
    {"test_parse_object_holding_embedded_objects()", test_parse_object_holding_embedded_objects},
    {"test_parse_object_holding_utf8_string()", test_parse_object_holding_utf8_string}, 
    {"test_parse_object_integer_value_in_next_chunk_followed_by_others()", test_parse_object_integer_value_in_next_chunk_followed_by_others}, 
    {"test_parse_object_key_input_in_two_chunks_1()", test_parse_object_key_input_in_two_chunks_1},
    {"test_parse_object_key_input_in_two_chunks_2()", test_parse_object_key_input_in_two_chunks_2},
    {"test_parse_object_key_input_in_two_chunks_3()", test_parse_object_key_input_in_two_chunks_3},
    {"test_parse_object_key_input_in_two_chunks_4()", test_parse_object_key_input_in_two_chunks_4},
    {"test_parse_object_string_input_in_two_chunks_1()", test_parse_object_string_input_in_two_chunks_1},
    {"test_parse_object_string_input_in_two_chunks_2()", test_parse_object_string_input_in_two_chunks_2},
    {"test_parse_object_string_input_in_two_chunks_3()", test_parse_object_string_input_in_two_chunks_3},
    {"test_parse_object_string_input_in_two_chunks_4()", test_parse_object_string_input_in_two_chunks_4},
    {"test_parse_object_integer_value_input_in_two_chunks()", test_parse_object_integer_value_input_in_two_chunks},
    {"test_parse_object_true_value_input_in_two_chunks_1()", test_parse_object_true_value_input_in_two_chunks_1},
    {"test_parse_object_true_value_input_in_two_chunks_2()", test_parse_object_true_value_input_in_two_chunks_2},
    {"test_parse_object_true_value_input_in_two_chunks_3()", test_parse_object_true_value_input_in_two_chunks_3},
    {"test_parse_object_false_value_input_in_two_chunks_1()", test_parse_object_false_value_input_in_two_chunks_1},
    {"test_parse_object_false_value_input_in_two_chunks_2()", test_parse_object_false_value_input_in_two_chunks_2},
    {"test_parse_object_false_value_input_in_two_chunks_3()", test_parse_object_false_value_input_in_two_chunks_3},
    {"test_parse_empty_array()", test_parse_empty_array},
    {"test_parse_array_holding_one_element()", test_parse_array_holding_one_element},
    {"test_parse_array_holding_two_elements()", test_parse_array_holding_two_elements},
    {"test_parse_array_holding_ordinary_elements()", test_parse_array_holding_ordinary_elements},
    {"test_parse_array_holding_empty_complex_elements()", test_parse_array_holding_empty_complex_elements},
    {"test_parse_array_holding_embedded_arrays()", test_parse_array_holding_embedded_arrays},
    CU_TEST_INFO_NULL
};

// ---- abnormal test case of parsing ----

void test_parse_object_without_enough_input_of_key_string(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_key\":123456,"};
    size_t buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    CU_ASSERT_FALSE(ret);

    if (!qn_err_json_is_need_more_text_input()) {
        CU_FAIL("The error is not `need more text input`.");
        return;
    } // if
}

void test_parse_object_without_enough_input_of_integer(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_key\":1"};
    size_t buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    CU_ASSERT_FALSE(ret);

    if (!qn_err_json_is_need_more_text_input()) {
        CU_FAIL("The error is not `need more text input`.");
        return;
    } // if
}

void test_parse_object_without_enough_input_of_value(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_key\":"};
    size_t buf_len = strlen(buf);
    qn_json_object_ptr obj_root = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse_object(prs, buf, &buf_len, &obj_root);
    qn_json_prs_destroy(prs);
    CU_ASSERT_FALSE(ret);

    if (!qn_err_json_is_need_more_text_input()) {
        CU_FAIL("The error is not `need more text input`.");
        return;
    } // if
}

CU_TestInfo test_abnormal_cases_of_json_parsing[] = {
    {"test_parse_object_without_enough_input_of_key_string()", test_parse_object_without_enough_input_of_key_string},
    {"test_parse_object_without_enough_input_of_integer()", test_parse_object_without_enough_input_of_integer},
    {"test_parse_object_without_enough_input_of_value()", test_parse_object_without_enough_input_of_value},
    CU_TEST_INFO_NULL
};

// ---- test formatter ----

void test_format_empty_object(void)
{
    qn_bool ret = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_create_object();
    CU_ASSERT_FATAL(obj_root != NULL);

    ret = qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 2);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{}", 2), 0);

    qn_json_destroy_object(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_immutable_object(void)
{
    qn_bool ret = qn_false;
    qn_json_object_ptr obj_root = qn_json_immutable_empty_object();
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    ret = qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 2);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{}", 2), 0);

    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_string_element(void)
{
    qn_bool ret = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_create_object();
    CU_ASSERT_FATAL(obj_root != NULL);

    ret = qn_json_set_string(obj_root, "_str", "Normal string");
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 24);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_str\":\"Normal string\"}", 24), 0);

    qn_json_destroy_object(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_integer_element(void)
{
    qn_bool ret = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_create_object();
    CU_ASSERT_FATAL(obj_root != NULL);

    ret = qn_json_set_integer(obj_root, "_int1", -123);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_set_integer(obj_root, "_int2", 987);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 26);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_int1\":-123,\"_int2\":987}", 26), 0);

    qn_json_destroy_object(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_number_element(void)
{
    qn_bool ret = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_create_object();
    CU_ASSERT_FATAL(obj_root != NULL);

    ret = qn_json_set_number(obj_root, "_num1", -123.123456);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_set_number(obj_root, "_num2", 987.987);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 40);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_num1\":-123.123456,\"_num2\":987.987000}", 40), 0);

    qn_json_destroy_object(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_boolean_element(void)
{
    qn_bool ret = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_create_object();
    CU_ASSERT_FATAL(obj_root != NULL);

    ret = qn_json_set_boolean(obj_root, "_bool1", qn_false);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_set_boolean(obj_root, "_bool2", qn_true);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 30);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_bool1\":false,\"_bool2\":true}", 30), 0);

    qn_json_destroy_object(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_null_element(void)
{
    qn_bool ret = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_create_object();
    CU_ASSERT_FATAL(obj_root != NULL);

    ret = qn_json_set_null(obj_root, "_null");
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 14);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_null\":null}", 14), 0);

    qn_json_destroy_object(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_empty_array(void)
{
    qn_bool ret = qn_false;
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_create_array();
    CU_ASSERT_FATAL(arr_root != NULL);

    ret = qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 2);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[]", 2), 0);

    qn_json_destroy_array(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_immutable_array(void)
{
    qn_bool ret = qn_false;
    qn_json_array_ptr arr_root = qn_json_immutable_empty_array();
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    ret = qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 2);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[]", 2), 0);

    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_string_element(void)
{
    qn_bool ret = qn_false;
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_create_array();
    CU_ASSERT_FATAL(arr_root != NULL);

    ret = qn_json_push_string(arr_root, "Normal string");
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 17);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[\"Normal string\"]", 17), 0);

    qn_json_destroy_array(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_integer_element(void)
{
    qn_bool ret = qn_false;
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_create_array();
    CU_ASSERT_FATAL(arr_root != NULL);

    ret = qn_json_push_integer(arr_root, -123);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_push_integer(arr_root, 987);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 10);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[-123,987]", 10), 0);

    qn_json_destroy_array(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_number_element(void)
{
    qn_bool ret = qn_false;
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_create_array();
    CU_ASSERT_FATAL(arr_root != NULL);

    ret = qn_json_unshift_number(arr_root, -123.123456);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_unshift_number(arr_root, 987.987);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 24);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[987.987000,-123.123456]", 24), 0);

    qn_json_destroy_array(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_boolean_element(void)
{
    qn_bool ret = qn_false;
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_create_array();
    CU_ASSERT_FATAL(arr_root != NULL);

    ret = qn_json_push_boolean(arr_root, qn_false);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_push_boolean(arr_root, qn_true);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 12);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[false,true]", 12), 0);

    qn_json_destroy_array(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_null_element(void)
{
    qn_bool ret = qn_false;
    qn_json_array_ptr arr_root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_create_array();
    CU_ASSERT_FATAL(arr_root != NULL);

    ret = qn_json_unshift_null(arr_root);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 6);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[null]", 6), 0);

    qn_json_destroy_array(arr_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_object_holding_complex_element(void)
{
    qn_bool ret = qn_false;
    qn_json_object_ptr obj_root = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj_root = qn_json_create_object();
    CU_ASSERT_FATAL(obj_root != NULL);

    arr_elem = qn_json_create_and_set_array(obj_root, "_arr");
    CU_ASSERT_FATAL(arr_elem != NULL);

    ret = qn_json_unshift_null(arr_elem);
    CU_ASSERT_TRUE(ret);

    obj_elem = qn_json_create_and_set_object(obj_root, "_obj");
    CU_ASSERT_FATAL(obj_elem != NULL);

    ret = qn_json_fmt_format_object(fmt, obj_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 25);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_arr\":[null],\"_obj\":{}}", 25), 0);

    qn_json_destroy_object(obj_root);
    qn_json_fmt_destroy(fmt);
}

void test_format_array_holding_complex_element(void)
{
    qn_bool ret = qn_false;
    qn_json_array_ptr arr_root = NULL;
    qn_json_object_ptr obj_elem = NULL;
    qn_json_array_ptr arr_elem = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    size_t buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    arr_root = qn_json_create_array();
    CU_ASSERT_FATAL(arr_root != NULL);

    obj_elem = qn_json_create_and_push_object(arr_root);
    CU_ASSERT_TRUE(obj_elem != NULL);

    ret = qn_json_set_null(obj_elem, "_null");
    CU_ASSERT_TRUE(ret);

    arr_elem = qn_json_create_and_push_array(arr_root);
    CU_ASSERT_FATAL(arr_elem != NULL);

    ret = qn_json_fmt_format_array(fmt, arr_root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 19);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[{\"_null\":null},[]]", 19), 0);

    qn_json_destroy_array(arr_root);
    qn_json_fmt_destroy(fmt);
}

CU_TestInfo test_normal_cases_of_json_formatting[] = {
    {"test_format_empty_object()", test_format_empty_object},
    {"test_format_immutable_object()", test_format_immutable_object},
    {"test_format_object_holding_string_element()", test_format_object_holding_string_element},
    {"test_format_object_holding_integer_element()", test_format_object_holding_integer_element},
    {"test_format_object_holding_number_element()", test_format_object_holding_number_element},
    {"test_format_object_holding_boolean_element()", test_format_object_holding_boolean_element},
    {"test_format_object_holding_null_element()", test_format_object_holding_null_element},
    {"test_format_empty_array()", test_format_empty_array},
    {"test_format_immutable_array()", test_format_immutable_array},
    {"test_format_array_holding_string_element()", test_format_array_holding_string_element},
    {"test_format_array_holding_integer_element()", test_format_array_holding_integer_element},
    {"test_format_array_holding_number_element()", test_format_array_holding_number_element},
    {"test_format_array_holding_boolean_element()", test_format_array_holding_boolean_element},
    {"test_format_array_holding_null_element()", test_format_array_holding_null_element},
    {"test_format_object_holding_complex_element()", test_format_object_holding_complex_element},
    {"test_format_array_holding_complex_element()", test_format_array_holding_complex_element},
    CU_TEST_INFO_NULL
};

// ---- test suites ----

CU_SuiteInfo suites[] = {
    {"test_normal_cases_of_json_manipulating", NULL, NULL, test_normal_cases_of_json_manipulating},
    {"test_normal_cases_of_json_parsing", NULL, NULL, test_normal_cases_of_json_parsing},
    {"test_abnormal_cases_of_json_parsing", NULL, NULL, test_abnormal_cases_of_json_parsing},
    {"test_normal_cases_of_json_formatting", NULL, NULL, test_normal_cases_of_json_formatting},
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
}
