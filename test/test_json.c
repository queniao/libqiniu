#include <CUnit/Basic.h>

#include "base/json.h"
#include "base/json_parser.h"
#include "base/json_formatter.h"

#define CU_ASSERT_LONG_DOUBLE_EQUAL(actual, expected, granularity) \
  { CU_assertImplementation(((fabsl((long double)(actual) - (expected)) <= fabsl((long double)(granularity)))), __LINE__, ("CU_ASSERT_LONG_DOUBLE_EQUAL(" #actual ","  #expected "," #granularity ")"), __FILE__, "", CU_FALSE); }

// ---- test functions ----

void test_manipulate_object(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr obj = NULL;
    qn_json_ptr elem = NULL;
    qn_string str = NULL;
    char buf[] = {"A line for creating string element."};
    qn_size buf_len = strlen(buf);

    obj = qn_json_create_object();
    CU_ASSERT_FATAL(obj != NULL);

    // set a string element
    ret = qn_json_set_string_raw(obj, "_str", buf, buf_len);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(obj), 1);

    // set a number element
    ret = qn_json_set_number(obj, "_num",-9.99L);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(obj), 2);

    // set a integer element
    ret = qn_json_set_integer(obj, "_int", 256);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(obj), 3);

    // set a null element
    ret = qn_json_set_null(obj, "_null");
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(obj), 4);

    // set a boolean element
    ret = qn_json_set_boolean(obj, "_false", qn_false);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(obj), 5);

    // check the null element
    qn_json_unset(obj, "_null");
    CU_ASSERT_EQUAL(qn_json_size(obj), 4);

    // check the integer element
    CU_ASSERT_EQUAL(qn_json_get_integer(obj, "_int", 0), 256);

    qn_json_unset(obj, "_int");
    CU_ASSERT_EQUAL(qn_json_size(obj), 3);

    // check the number element
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_get_number(obj, "_num", 0.0L), -9.99L, 0.01L);
    CU_ASSERT_EQUAL(qn_json_size(obj), 3);

    // check the string element
    str = qn_json_get_string(obj, "_str", NULL);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), buf) == 0);
    CU_ASSERT_EQUAL(qn_json_size(obj), 3);

    qn_json_destroy(obj);
} // test_manipulate_object

void test_manipulate_array(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr arr = NULL;
    qn_json_ptr elem = NULL;
    qn_string str = NULL;
    char buf[] = {"A line for creating string element."};
    qn_size buf_len = strlen(buf);

    arr = qn_json_create_array();
    CU_ASSERT_FATAL(arr != NULL);

    // unshift a string element
    ret = qn_json_unshift_string_raw(arr, buf, buf_len);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(arr), 1);

    // push a number element
    ret = qn_json_push_number(arr, -9.99L);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(arr), 2);

    // push a integer element
    ret = qn_json_push_integer(arr, 256);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(arr), 3);

    // unshift a null element
    ret = qn_json_unshift_null(arr);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(arr), 4);

    // push a boolean element
    ret = qn_json_push_boolean(arr, qn_false);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL(qn_json_size(arr), 5);

    // check the first element (null)
    qn_json_shift(arr);
    CU_ASSERT_EQUAL(qn_json_size(arr), 4);

    // check the second element (string)
    str = qn_json_pick_string(arr, 0, NULL);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), buf) == 0);

    qn_json_shift(arr);
    CU_ASSERT_EQUAL(qn_json_size(arr), 3);

    // check the last element (boolean == false)
    CU_ASSERT_TRUE(qn_json_pick_boolean(arr, 2, qn_true) == qn_false);
    qn_json_pop(arr);
    CU_ASSERT_EQUAL(qn_json_size(arr), 2);

    // check the last element (int == 256)
    CU_ASSERT_TRUE(qn_json_pick_integer(arr, 1, 0) != 0);
    CU_ASSERT_TRUE(qn_json_pick_integer(arr, 1, 0) == 256);
    CU_ASSERT_EQUAL(qn_json_size(arr), 2);

    // check the first element
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_pick_number(arr, 0, 0.0L), -9.99L, 0.01L);
    CU_ASSERT_EQUAL(qn_json_size(arr), 2);

    qn_json_destroy(arr);
} // test_manipulate_array

CU_TestInfo test_normal_cases_of_json_manipulating[] = {
    {"test_manipulate_object()", test_manipulate_object},
    {"test_manipulate_array()", test_manipulate_array},
    CU_TEST_INFO_NULL
};
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
    qn_string str = NULL;
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

    str = qn_json_get_string(obj, "trivial", NULL);
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

    CU_ASSERT_TRUE(qn_json_get_integer(obj, "int", 0) == -123);

    qn_json_destroy(obj);
} // test_parse_object_holding_two_elements

void test_parse_object_holding_ordinary_elements(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_num\":+123.456,\"_true\":true,\"_false\":false,\"_null\":null}"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr obj = NULL;
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

    CU_ASSERT_TRUE(qn_json_get_boolean(obj, "_false", qn_true) == qn_false);
    CU_ASSERT_TRUE(qn_json_get_boolean(obj, "_true", qn_false) == qn_true);
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_get_number(obj, "_num", 0.0L), 123.456L, 0.001L);

    qn_json_destroy(obj);
} // test_parse_object_holding_ordinary_elements

void test_parse_object_holding_empty_complex_elements(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_arr\":[],\"_obj\":{}}"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr obj = NULL;
    qn_json_ptr elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &obj);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding empty complex elements.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_object(obj));
    CU_ASSERT_TRUE(!qn_json_is_empty(obj));

    elem = qn_json_get_array(obj, "_arr", NULL);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_array(elem));
    CU_ASSERT_TRUE(qn_json_is_empty(elem));

    elem = qn_json_get_object(obj, "_obj", NULL);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_object(elem));
    CU_ASSERT_TRUE(qn_json_is_empty(elem));

    qn_json_destroy(obj);
} // test_parse_object_holding_empty_complex_elements

void test_parse_object_holding_embedded_objects(void)
{
    qn_bool ret;
    const char buf[] = {"{\"_obj\":{\"_num\":+123.456,\"_true\":true,\"_false\":false,\"_null\":null},\"_obj2\":{}}"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr obj = NULL;
    qn_json_ptr child = NULL;
    qn_number num_val = 0.0L;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &obj);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the object holding embedded objects.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_object(obj));
    CU_ASSERT_TRUE(!qn_json_is_empty(obj));

    child = qn_json_get_object(obj, "_obj", NULL);

    CU_ASSERT_TRUE(child != NULL);
    CU_ASSERT_TRUE(qn_json_is_object(child));
    CU_ASSERT_TRUE(!qn_json_is_empty(child));

    CU_ASSERT_TRUE(qn_json_get_boolean(child, "_false", qn_true) == qn_false);
    CU_ASSERT_TRUE(qn_json_get_boolean(child, "_true", qn_false) == qn_true);

    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_get_number(child, "_num", 0.0L), 123.456L, 0.001L);

    child = qn_json_get_object(obj, "_obj2", NULL);

    CU_ASSERT_TRUE(child != NULL);
    CU_ASSERT_TRUE(qn_json_is_object(child));
    CU_ASSERT_TRUE(qn_json_is_empty(child));

    qn_json_destroy(obj);
} // test_parse_object_holding_embedded_objects

void test_parse_empty_array(void)
{
    qn_bool ret;
    const char buf[] = {"[]"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr arr = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &arr);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the empty array.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_array(arr));
    CU_ASSERT_TRUE(qn_json_is_empty(arr));

    qn_json_destroy(arr);
} // test_parse_empty_array

void test_parse_array_holding_one_element(void)
{
    qn_bool ret;
    const char buf[] = {"[\"This is a trivial element.\"]"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr arr = NULL;
    qn_string str = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &arr);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding one element.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_array(arr));
    CU_ASSERT_TRUE(!qn_json_is_empty(arr));

    str = qn_json_pick_string(arr, 0, NULL);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), "This is a trivial element.") == 0);

    qn_json_destroy(arr);
} // test_parse_array_holding_one_element

void test_parse_array_holding_two_elements(void)
{
    qn_bool ret;
    const char buf[] = {"[\"This is a trivial element.\",-123]"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr arr = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &arr);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding two elements.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_array(arr));
    CU_ASSERT_TRUE(!qn_json_is_empty(arr));
    CU_ASSERT_TRUE(qn_json_pick_integer(arr, 1, 0) == -123);

    qn_json_destroy(arr);
} // test_parse_array_holding_two_elements

void test_parse_array_holding_ordinary_elements(void)
{
    qn_bool ret;
    const char buf[] = {"[+123.456,true,false,null]"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr arr = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &arr);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding ordinary elements.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_array(arr));
    CU_ASSERT_TRUE(!qn_json_is_empty(arr));

    CU_ASSERT_TRUE(qn_json_pick_boolean(arr, 2, qn_true) == qn_false);
    CU_ASSERT_TRUE(qn_json_pick_boolean(arr, 1, qn_false) == qn_true);
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_pick_number(arr, 0, 0.0L), 123.456L, 0.001L);

    qn_json_destroy(arr);
} // test_parse_array_holding_ordinary_elements

void test_parse_array_holding_empty_complex_elements(void)
{
    qn_bool ret;
    const char buf[] = {"[{},[]]"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr arr = NULL;
    qn_json_ptr elem = NULL;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &arr);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding empty complex elements.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_array(arr));
    CU_ASSERT_TRUE(!qn_json_is_empty(arr));

    elem = qn_json_pick_object(arr, 0, NULL);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_object(elem));
    CU_ASSERT_TRUE(qn_json_is_empty(elem));

    elem = qn_json_pick_array(arr, 1, NULL);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_array(elem));
    CU_ASSERT_TRUE(qn_json_is_empty(elem));

    qn_json_destroy(arr);
} // test_parse_array_holding_empty_complex_elements

void test_parse_array_holding_embedded_arrays(void)
{
    qn_bool ret;
    const char buf[] = {"[[+123.456,true,false,null],[]]"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr arr = NULL;
    qn_json_ptr child = NULL;
    qn_json_ptr elem = NULL;
    qn_number num_val = 0.0L;
    qn_json_parser_ptr prs = NULL;

    prs = qn_json_prs_create();
    CU_ASSERT_FATAL(prs != NULL);

    ret = qn_json_prs_parse(prs, buf, &buf_len, &arr);
    qn_json_prs_destroy(prs);
    if (!ret) {
        CU_FAIL("Cannot parse the array holding embedded arrays.");
        return;
    } // if

    CU_ASSERT_TRUE(qn_json_is_array(arr));
    CU_ASSERT_TRUE(!qn_json_is_empty(arr));

    child = qn_json_pick_array(arr, 0, NULL);

    CU_ASSERT_TRUE(child != NULL);
    CU_ASSERT_TRUE(qn_json_is_array(child));
    CU_ASSERT_TRUE(!qn_json_is_empty(child));

    CU_ASSERT_TRUE(qn_json_pick_boolean(child, 2, qn_true) == qn_false);
    CU_ASSERT_TRUE(qn_json_pick_boolean(child, 1, qn_false) == qn_true);
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_pick_number(child, 0, 0.0L), 123.456L, 0.001L);

    child = qn_json_pick_array(arr, 1, NULL);

    CU_ASSERT_TRUE(child != NULL);
    CU_ASSERT_TRUE(qn_json_is_array(child));
    CU_ASSERT_TRUE(qn_json_is_empty(child));

    qn_json_destroy(arr);
} // test_parse_array_holding_embedded_arrays

CU_TestInfo test_normal_cases_of_json_parsing[] = {
    {"test_parse_empty_object()", test_parse_empty_object},
    {"test_parse_object_holding_one_element()", test_parse_object_holding_one_element},
    {"test_parse_object_holding_two_elements()", test_parse_object_holding_two_elements},
    {"test_parse_object_holding_ordinary_elements()", test_parse_object_holding_ordinary_elements},
    {"test_parse_object_holding_empty_complex_elements()", test_parse_object_holding_empty_complex_elements},
    {"test_parse_object_holding_embedded_objects()", test_parse_object_holding_embedded_objects},
    {"test_parse_empty_array()", test_parse_empty_array},
    {"test_parse_array_holding_one_element()", test_parse_array_holding_one_element},
    {"test_parse_array_holding_two_elements()", test_parse_array_holding_two_elements},
    {"test_parse_array_holding_ordinary_elements()", test_parse_array_holding_ordinary_elements},
    {"test_parse_array_holding_empty_complex_elements()", test_parse_array_holding_empty_complex_elements},
    {"test_parse_array_holding_embedded_arrays()", test_parse_array_holding_embedded_arrays},
    CU_TEST_INFO_NULL
};

// ---- test formatter ----

void test_format_empty_object(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_object();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 2);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{}", 2), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_empty_object

void test_format_object_holding_string_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_ptr str = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_object();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_set_string(root, "_str", "Normal string");
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 24);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_str\":\"Normal string\"}", 24), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_object_holding_string_element

void test_format_object_holding_integer_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_object();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_set_integer(root, "_int1", -123);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_set_integer(root, "_int2", 987);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 26);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_int1\":-123,\"_int2\":987}", 26), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_object_holding_integer_element

void test_format_object_holding_number_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_object();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_set_number(root, "_num1", -123.123456);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_set_number(root, "_num2", 987.987);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 40);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_num1\":-123.123456,\"_num2\":987.987000}", 40), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_object_holding_number_element

void test_format_object_holding_boolean_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_object();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_set_boolean(root, "_bool1", qn_false);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_set_boolean(root, "_bool2", qn_true);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 30);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_bool1\":false,\"_bool2\":true}", 30), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_object_holding_boolean_element

void test_format_object_holding_null_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_object();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_set_null(root, "_null");
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 14);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_null\":null}", 14), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_object_holding_null_element

void test_format_empty_array(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_array();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 2);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[]", 2), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_empty_array

void test_format_array_holding_string_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_array();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_push_string(root, "Normal string");
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 17);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[\"Normal string\"]", 17), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_array_holding_string_element

void test_format_array_holding_integer_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_array();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_push_integer(root, -123);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_push_integer(root, 987);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 10);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[-123,987]", 10), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_array_holding_integer_element

void test_format_array_holding_number_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_array();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_unshift_number(root, -123.123456);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_unshift_number(root, 987.987);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 24);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[987.987000,-123.123456]", 24), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_array_holding_number_element

void test_format_array_holding_boolean_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_array();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_push_boolean(root, qn_false);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_push_boolean(root, qn_true);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 12);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[false,true]", 12), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_array_holding_boolean_element

void test_format_array_holding_null_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_array();
    CU_ASSERT_FATAL(root != NULL);

    ret = qn_json_unshift_null(root);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 6);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[null]", 6), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_array_holding_null_element

void test_format_object_holding_complex_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_ptr arr = NULL;
    qn_json_ptr elem = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_object();
    CU_ASSERT_FATAL(root != NULL);

    arr = qn_json_create_and_set_array(root, "_arr");
    CU_ASSERT_FATAL(arr != NULL);

    ret = qn_json_unshift_null(arr);
    CU_ASSERT_TRUE(ret);

    elem = qn_json_create_and_set_object(root, "_obj");
    CU_ASSERT_FATAL(elem != NULL);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 25);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_arr\":[null],\"_obj\":{}}", 25), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_object_holding_complex_element

void test_format_array_holding_complex_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr root = NULL;
    qn_json_ptr obj = NULL;
    qn_json_ptr elem = NULL;
    qn_json_formatter_ptr fmt = NULL;
    char buf[128];
    qn_size buf_size = sizeof(buf);

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    root = qn_json_create_array();
    CU_ASSERT_FATAL(root != NULL);

    obj = qn_json_create_and_push_object(root);
    CU_ASSERT_TRUE(obj != NULL);

    ret = qn_json_set_null(obj, "_null");
    CU_ASSERT_TRUE(ret);

    elem = qn_json_create_and_push_array(root);
    CU_ASSERT_FATAL(elem != NULL);

    ret = qn_json_fmt_format(fmt, root, buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 19);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "[{\"_null\":null},[]]", 19), 0);

    qn_json_destroy(root);
    qn_json_fmt_destroy(fmt);
} // test_format_object_holding_complex_element

CU_TestInfo test_normal_cases_of_json_formatting[] = {
    {"test_format_empty_object()", test_format_empty_object},
    {"test_format_object_holding_string_element()", test_format_object_holding_string_element},
    {"test_format_object_holding_integer_element()", test_format_object_holding_integer_element},
    {"test_format_object_holding_number_element()", test_format_object_holding_number_element},
    {"test_format_object_holding_boolean_element()", test_format_object_holding_boolean_element},
    {"test_format_object_holding_null_element()", test_format_object_holding_null_element},
    {"test_format_empty_array()", test_format_empty_array},
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
} // main
