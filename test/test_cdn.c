#include <CUnit/Basic.h> 

#include "qiniu/base/string.h"
#include "qiniu/cdn.h"

// ---- test extern functions ----

void test_cdn_make_dnurl(void)
{
    char * key = "12345678";
    qn_uint32 deadline = 1438358400;
    qn_string dnurl;

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com?sign=36f8ae9dd7032b1efae2b00f530a655a&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com?v=1.1", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com?v=1.1&sign=36f8ae9dd7032b1efae2b00f530a655a&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com?imageView2/1/w/100/h/100", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com?imageView2/1/w/100/h/100&sign=36f8ae9dd7032b1efae2b00f530a655a&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com/", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com/?sign=2acd086896dad6eb1824187b199e4841&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com/?v=1.1", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com/?v=1.1&sign=2acd086896dad6eb1824187b199e4841&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com/?imageView2/1/w/100/h/100", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com/?imageView2/1/w/100/h/100&sign=2acd086896dad6eb1824187b199e4841&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com//", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com//?sign=c6344e30d654a628586493d1095f3cb6&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com//?v=1.1", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com//?v=1.1&sign=c6344e30d654a628586493d1095f3cb6&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com//?imageView2/1/w/100/h/100", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com//?imageView2/1/w/100/h/100&sign=c6344e30d654a628586493d1095f3cb6&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com/DIR1/dir2/vodfile.mp4?v=1.1", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com/DIR1/dir2/vodfile.mp4?v=1.1&sign=19eb212771e87cc3d478b9f32d6c7bf9&t=55bb9b80");
    qn_str_destroy(dnurl);

    dnurl = qn_cdn_make_dnurl_with_deadline(key, "http://xxx.yyy.com/DIR1/中文/vodfile.mp4?v=1.2", deadline);
    CU_ASSERT_PTR_NOT_NULL(qn_str_cstr(dnurl));
    CU_ASSERT_STRING_EQUAL(qn_str_cstr(dnurl), "http://xxx.yyy.com/DIR1/%E4%B8%AD%E6%96%87/vodfile.mp4?v=1.2&sign=6356bca0d2aecf7211003e468861f5ea&t=55bb9b80");
    qn_str_destroy(dnurl);
}

CU_TestInfo test_normal_cases_of_extern_functions[] = {
    {"test_cdn_make_dnurl()", test_cdn_make_dnurl},
    CU_TEST_INFO_NULL
};

CU_SuiteInfo suites[] = {
    {"test_normal_cases_of_extern_functions", NULL, NULL, test_normal_cases_of_extern_functions},
    CU_SUITE_INFO_NULL
};

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    } // if

    pSuite = CU_add_suite("Suite_Test_CDN", NULL, NULL);
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
