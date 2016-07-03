#include <CUnit/Basic.h>

#include "base/json.h"

#define CU_ASSERT_LONG_DOUBLE_EQUAL(actual, expected, granularity) \
  { CU_assertImplementation(((fabsl((long double)(actual) - (expected)) <= fabsl((long double)(granularity)))), __LINE__, ("CU_ASSERT_LONG_DOUBLE_EQUAL(" #actual ","  #expected "," #granularity ")"), __FILE__, "", CU_FALSE); }

// ---- test functions ----

void test_manipulate_object(void)
{
    qn_json_ptr obj = NULL;
    qn_json_ptr new_elem = NULL;
    qn_json_ptr elem = NULL;
    qn_string_ptr str = NULL;
    char buf[] = {"A line for creating string element."};
    qn_size buf_len = strlen(buf);

    obj = qn_json_create_object();
    CU_ASSERT_FATAL(obj != NULL);

    // set a string element
    new_elem = qn_json_create_string(buf, buf_len);
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_set(obj, "_str", new_elem);
    CU_ASSERT_EQUAL(qn_json_size(obj), 1);

    // set a number element
    new_elem = qn_json_create_number(-9.99L);
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_set(obj, "_num", new_elem);
    CU_ASSERT_EQUAL(qn_json_size(obj), 2);

    // set a integer element
    new_elem = qn_json_create_integer(256);
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_set(obj, "_int", new_elem);
    CU_ASSERT_EQUAL(qn_json_size(obj), 3);

    // set a null element
    new_elem = qn_json_create_null();
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_set(obj, "_null", new_elem);
    CU_ASSERT_EQUAL(qn_json_size(obj), 4);

    // set a boolean element
    new_elem = qn_json_create_boolean(qn_false);
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_set(obj, "_false", new_elem);
    CU_ASSERT_EQUAL(qn_json_size(obj), 5);

    // check the null element
    elem = qn_json_get(obj, "_null");
    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_null(elem));

    qn_json_unset(obj, "_null");
    CU_ASSERT_EQUAL(qn_json_size(obj), 4);

    // check the integer element
    elem = qn_json_get(obj, "_int");
    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_integer(elem));
    CU_ASSERT_EQUAL(qn_json_to_integer(elem), 256);

    qn_json_unset(obj, "_int");
    CU_ASSERT_EQUAL(qn_json_size(obj), 3);

    // check the number element
    elem = qn_json_get(obj, "_num");
    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_number(elem));
    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_to_number(elem), -9.99L, 0.01L);
    CU_ASSERT_EQUAL(qn_json_size(obj), 3);

    // check the string element
    elem = qn_json_get(obj, "_str");
    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_string(elem));

    str = qn_json_to_string(elem);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), buf) == 0);
    CU_ASSERT_EQUAL(qn_json_size(obj), 3);

    qn_json_destroy(obj);
} // test_manipulate_object

void test_manipulate_array(void)
{
    qn_json_ptr arr = NULL;
    qn_json_ptr new_elem = NULL;
    qn_json_ptr elem = NULL;
    qn_string_ptr str = NULL;
    char buf[] = {"A line for creating string element."};
    qn_size buf_len = strlen(buf);

    arr = qn_json_create_array();

    CU_ASSERT_FATAL(arr != NULL);

    // unshift a string element
    new_elem = qn_json_create_string(buf, buf_len);
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_unshift(arr, new_elem);
    CU_ASSERT_EQUAL(qn_json_size(arr), 1);

    // push a number element
    new_elem = qn_json_create_number(-9.99L);
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_push(arr, new_elem);
    CU_ASSERT_EQUAL(qn_json_size(arr), 2);

    // push a integer element
    new_elem = qn_json_create_integer(256);
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_push(arr, new_elem);
    CU_ASSERT_EQUAL(qn_json_size(arr), 3);

    // unshift a null element
    new_elem = qn_json_create_null();
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_unshift(arr, new_elem);
    CU_ASSERT_EQUAL(qn_json_size(arr), 4);

    // push a boolean element
    new_elem = qn_json_create_boolean(qn_false);
    CU_ASSERT_FATAL(new_elem != NULL);

    qn_json_push(arr, new_elem);
    CU_ASSERT_EQUAL(qn_json_size(arr), 5);

    // check the first element (null)
    elem = qn_json_get_at(arr, 0);
    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_null(elem));

    qn_json_shift(arr);
    CU_ASSERT_EQUAL(qn_json_size(arr), 4);

    // check the second element (string)
    elem = qn_json_get_at(arr, 0);
    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_string(elem));

    str = qn_json_to_string(elem);
    CU_ASSERT_TRUE(str != NULL);
    CU_ASSERT_TRUE(strcmp(qn_str_cstr(str), buf) == 0);

    qn_json_shift(arr);
    CU_ASSERT_EQUAL(qn_json_size(arr), 3);

    // check the last element (boolean == false)
    elem = qn_json_get_at(arr, 2);
    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_boolean(elem));
    CU_ASSERT(qn_json_to_boolean(elem) == qn_false);

    qn_json_pop(arr);
    CU_ASSERT_EQUAL(qn_json_size(arr), 2);

    // check the last element (int == 256)
    elem = qn_json_get_at(arr, 1);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_integer(elem));

    CU_ASSERT_EQUAL(qn_json_to_integer(elem), 256);
    CU_ASSERT_EQUAL(qn_json_size(arr), 2);

    // check the first element
    elem = qn_json_get_at(arr, 0);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_number(elem));

    CU_ASSERT_LONG_DOUBLE_EQUAL(qn_json_to_number(elem), -9.99L, 0.01L);
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

    elem = qn_json_get(obj, "_arr");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_array(elem));
    CU_ASSERT_TRUE(qn_json_is_empty(elem));

    elem = qn_json_get(obj, "_obj");

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
    qn_json_ptr child_obj = NULL;
    qn_json_ptr elem = NULL;
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

    child_obj = qn_json_get(obj, "_obj");

    CU_ASSERT_TRUE(child_obj != NULL);
    CU_ASSERT_TRUE(qn_json_is_object(child_obj));
    CU_ASSERT_TRUE(!qn_json_is_empty(child_obj));

    elem = qn_json_get(child_obj, "_null");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_null(elem));

    elem = qn_json_get(child_obj, "_false");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_boolean(elem));
    CU_ASSERT_TRUE(qn_json_to_boolean(elem) == false);

    elem = qn_json_get(child_obj, "_true");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_boolean(elem));
    CU_ASSERT_TRUE(qn_json_to_boolean(elem) == true);

    elem = qn_json_get(child_obj, "_num");

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_number(elem));

    num_val = qn_json_to_number(elem);

    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 123.456L, 0.001L);

    child_obj = qn_json_get(obj, "_obj2");

    CU_ASSERT_TRUE(child_obj != NULL);
    CU_ASSERT_TRUE(qn_json_is_object(child_obj));
    CU_ASSERT_TRUE(qn_json_is_empty(child_obj));

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
    qn_json_ptr elem = NULL;
    qn_string_ptr str = NULL;
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

    elem = qn_json_get_at(arr, 0);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_string(elem));

    str = qn_json_to_string(elem);
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
    qn_json_ptr elem = NULL;
    qn_integer val = 0;
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

    elem = qn_json_get_at(arr, 1);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_integer(elem));

    val = qn_json_to_integer(elem);
    CU_ASSERT_TRUE(val == -123);

    qn_json_destroy(arr);
} // test_parse_array_holding_two_elements

void test_parse_array_holding_ordinary_elements(void)
{
    qn_bool ret;
    const char buf[] = {"[+123.456,true,false,null]"};
    qn_size buf_len = strlen(buf);
    qn_json_ptr arr = NULL;
    qn_json_ptr elem = NULL;
    qn_number num_val = 0.0L;
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

    elem = qn_json_get_at(arr, 3);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_null(elem));

    elem = qn_json_get_at(arr, 2);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_boolean(elem));
    CU_ASSERT_TRUE(qn_json_to_boolean(elem) == false);

    elem = qn_json_get_at(arr, 1);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_boolean(elem));
    CU_ASSERT_TRUE(qn_json_to_boolean(elem) == true);

    elem = qn_json_get_at(arr, 0);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_number(elem));

    num_val = qn_json_to_number(elem);

    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 123.456L, 0.001L);

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

    elem = qn_json_get_at(arr, 0);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_object(elem));
    CU_ASSERT_TRUE(qn_json_is_empty(elem));

    elem = qn_json_get_at(arr, 1);

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
    qn_json_ptr child_obj = NULL;
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

    child_obj = qn_json_get_at(arr, 0);

    CU_ASSERT_TRUE(child_obj != NULL);
    CU_ASSERT_TRUE(qn_json_is_array(child_obj));
    CU_ASSERT_TRUE(!qn_json_is_empty(child_obj));

    elem = qn_json_get_at(child_obj, 3);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_null(elem));

    elem = qn_json_get_at(child_obj, 2);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_boolean(elem));
    CU_ASSERT_TRUE(qn_json_to_boolean(elem) == false);

    elem = qn_json_get_at(child_obj, 1);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_boolean(elem));
    CU_ASSERT_TRUE(qn_json_to_boolean(elem) == true);

    elem = qn_json_get_at(child_obj, 0);

    CU_ASSERT_TRUE(elem != NULL);
    CU_ASSERT_TRUE(qn_json_is_number(elem));

    num_val = qn_json_to_number(elem);

    CU_ASSERT_LONG_DOUBLE_EQUAL(num_val, 123.456L, 0.001L);

    child_obj = qn_json_get_at(arr, 1);

    CU_ASSERT_TRUE(child_obj != NULL);
    CU_ASSERT_TRUE(qn_json_is_array(child_obj));
    CU_ASSERT_TRUE(qn_json_is_empty(child_obj));

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
    qn_json_ptr obj = NULL;
    qn_json_formatter_ptr fmt = NULL;
    const char * buf = NULL;
    qn_size buf_size = 0;

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj = qn_json_create_object();
    CU_ASSERT_FATAL(obj != NULL);

    ret = qn_json_fmt_format(fmt, obj, &buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 2);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{}", 2), 0);

    qn_json_destroy(obj);
    qn_json_fmt_destroy(fmt);
} // test_format_empty_object

void test_format_object_holding_string_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr obj = NULL;
    qn_json_ptr str = NULL;
    qn_json_formatter_ptr fmt = NULL;
    const char * buf = NULL;
    qn_size buf_size = 0;

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj = qn_json_create_object();
    CU_ASSERT_FATAL(obj != NULL);

    str = qn_json_create_string("Normal string", 13);
    CU_ASSERT_FATAL(str != NULL);

    ret = qn_json_set(obj, "_str", str);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, obj, &buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 24);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_str\":\"Normal string\"}", 24), 0);

    qn_json_destroy(obj);
    qn_json_fmt_destroy(fmt);
} // test_format_object_holding_string_element

void test_format_object_holding_integer_element(void)
{
    qn_bool ret = qn_false;
    qn_json_ptr obj = NULL;
    qn_json_ptr integer = NULL;
    qn_json_formatter_ptr fmt = NULL;
    const char * buf = NULL;
    qn_size buf_size = 0;

    fmt = qn_json_fmt_create();
    CU_ASSERT_FATAL(fmt != NULL);

    obj = qn_json_create_object();
    CU_ASSERT_FATAL(obj != NULL);

    integer = qn_json_create_integer(-123);
    CU_ASSERT_FATAL(integer != NULL);

    ret = qn_json_set(obj, "_int1", integer);
    CU_ASSERT_TRUE(ret);

    integer = qn_json_create_integer(987);
    CU_ASSERT_FATAL(integer != NULL);

    ret = qn_json_set(obj, "_int2", integer);
    CU_ASSERT_TRUE(ret);

    ret = qn_json_fmt_format(fmt, obj, &buf, &buf_size);
    CU_ASSERT_TRUE(ret);
    CU_ASSERT_EQUAL_FATAL(buf_size, 26);
    CU_ASSERT_EQUAL_FATAL(memcmp(buf, "{\"_int1\":-123,\"_int2\":987}", 26), 0);

    qn_json_destroy(obj);
    qn_json_fmt_destroy(fmt);
} // test_format_object_holding_integer_element

CU_TestInfo test_normal_cases_of_json_formatting[] = {
    {"test_format_empty_object()", test_format_empty_object},
    {"test_format_object_holding_string_element()", test_format_object_holding_string_element},
    {"test_format_object_holding_integer_element()", test_format_object_holding_integer_element},
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
