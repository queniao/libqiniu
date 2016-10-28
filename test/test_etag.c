#include <CUnit/Basic.h> 

#include "qiniu/base/string.h"
#include "qiniu/etag.c"

// ---- test extern functions ----

void test_context_create_and_destroy(void)
{
    qn_etag_context_ptr new_ctx = qn_etag_ctx_create();
    CU_ASSERT_PTR_NOT_NULL(new_ctx);

    qn_etag_ctx_destroy(new_ctx);
}

CU_TestInfo test_normal_cases_of_extern_functions[] = {
    {"test_context_create_and_destroy()", test_context_create_and_destroy},
    CU_TEST_INFO_NULL
};

// ----

void test_buffer_size_equals_zero_byte(void)
{
    qn_string digest;
    char buf[16];

    memset(buf, 0, sizeof(buf));
    
    digest = qn_etag_digest_buffer(buf, 0);

    CU_ASSERT_PTR_NOT_NULL(digest);
    CU_ASSERT_STRING_EQUAL(digest, "Fto5o-5ea0sNMlW_75VgGJCv2AcJ");
}

void test_buffer_size_equals_one_byte(void)
{
    qn_string digest;
    char buf[16];

    memset(buf, 0, sizeof(buf));
    
    digest = qn_etag_digest_buffer(buf, 1);

    CU_ASSERT_PTR_NOT_NULL(digest);
    CU_ASSERT_STRING_EQUAL(digest, "FlupPJ2wz_k_UrUh10IOQ_btonhP");
}

void test_buffer_size_one_byte_less_than_one_block(void)
{
    qn_string digest;
    int buf_size = (1 << 22) - 1;
    char * buf = malloc(buf_size);

    memset(buf, 0, buf_size);
    
    digest = qn_etag_digest_buffer(buf, buf_size);
    free(buf);

    CU_ASSERT_PTR_NOT_NULL(digest);
    CU_ASSERT_STRING_EQUAL(digest, "FojtiN9WHTdzhzAV4wrBC76RaWV0");
}

void test_buffer_size_equals_one_block(void)
{
    qn_string digest;
    int buf_size = (1 << 22);
    char * buf = malloc(buf_size);

    memset(buf, 0, buf_size);
    
    digest = qn_etag_digest_buffer(buf, buf_size);
    free(buf);

    CU_ASSERT_PTR_NOT_NULL(digest);
    CU_ASSERT_STRING_EQUAL(digest, "FivMvS848VwT631aif2dhfWV4jvD");
}

void test_buffer_size_one_byte_greater_than_one_block(void)
{
    qn_string digest;
    int buf_size = (1 << 22) + 1;
    char * buf = malloc(buf_size);

    memset(buf, 0, buf_size);
    
    digest = qn_etag_digest_buffer(buf, buf_size);
    free(buf);

    CU_ASSERT_PTR_NOT_NULL(digest);
    CU_ASSERT_STRING_EQUAL(digest, "lhCFgki5yzon0rjN9uJusf6qtsF6");
}

void test_buffer_size_one_byte_less_than_two_blocks(void)
{
    qn_string digest;
    int buf_size = (1 << 22) * 2 - 1;
    char * buf = malloc(buf_size);

    memset(buf, 0, buf_size);
    
    digest = qn_etag_digest_buffer(buf, buf_size);
    free(buf);

    CU_ASSERT_PTR_NOT_NULL(digest);
    CU_ASSERT_STRING_EQUAL(digest, "lvILQZ30fvbgKhyL83FHZ5dl4d6q");
}

void test_buffer_size_equals_two_blocks(void)
{
    qn_string digest;
    int buf_size = (1 << 22) * 2;
    char * buf = malloc(buf_size);

    memset(buf, 0, buf_size);
    
    digest = qn_etag_digest_buffer(buf, buf_size);
    free(buf);

    CU_ASSERT_PTR_NOT_NULL(digest);
    CU_ASSERT_STRING_EQUAL(digest, "lsCVE24-Immdd6zm-ffVVhsWYcDG");
}

void test_buffer_size_one_byte_greater_than_two_blocks(void)
{
    qn_string digest;
    int buf_size = (1 << 22) * 2 + 1;
    char * buf = malloc(buf_size);

    memset(buf, 0, buf_size);
    
    digest = qn_etag_digest_buffer(buf, buf_size);
    free(buf);

    CU_ASSERT_PTR_NOT_NULL(digest);
    CU_ASSERT_STRING_EQUAL(digest, "ljw1EdDAXqxjBRbylrCwR_xmDXdU");
}

void test_buffer_size_one_byte_greater_than_three_blocks(void)
{
    qn_string digest;
    int buf_size = (1 << 22) * 2 + (1 << 22) + 1;
    char * buf = malloc(buf_size);

    memset(buf, 0, buf_size);
    
    digest = qn_etag_digest_buffer(buf, buf_size);
    free(buf);

    CU_ASSERT_PTR_NOT_NULL(digest);
    CU_ASSERT_STRING_EQUAL(digest, "lvJcAt9M54FOUTGLz0jIOR_inhfs");
}

CU_TestInfo test_normal_cases_of_buffer_etags[] = {
    {"test_buffer_size_equals_zero_byte()", test_buffer_size_equals_zero_byte},
    {"test_buffer_size_equals_one_byte()", test_buffer_size_equals_one_byte},
    {"test_buffer_size_one_byte_less_than_one_block()", test_buffer_size_one_byte_less_than_one_block},
    {"test_buffer_size_equals_one_block()", test_buffer_size_equals_one_block},
    {"test_buffer_size_one_byte_greater_than_one_block()", test_buffer_size_one_byte_greater_than_one_block},
    {"test_buffer_size_one_byte_less_than_two_blocks()", test_buffer_size_one_byte_less_than_two_blocks},
    {"test_buffer_size_equals_two_blocks()", test_buffer_size_equals_two_blocks},
    {"test_buffer_size_one_byte_greater_than_two_blocks()", test_buffer_size_one_byte_greater_than_two_blocks},
    {"test_buffer_size_one_byte_greater_than_three_blocks()", test_buffer_size_one_byte_greater_than_three_blocks},
    CU_TEST_INFO_NULL
};

// ---- test suites ----

CU_SuiteInfo suites[] = {
    {"test_normal_cases_of_extern_functions", NULL, NULL, test_normal_cases_of_extern_functions},
    {"test_normal_cases_of_buffer_etags", NULL, NULL, test_normal_cases_of_buffer_etags},
    CU_SUITE_INFO_NULL
};

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    } // if

    pSuite = CU_add_suite("Suite_Test_ETAG", NULL, NULL);
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
