add_executable (test_string test_string.c
    ../src/qiniu/base/errors.c
    ../src/qiniu/base/base64.c
    ../src/qiniu/base/string.c)
target_link_libraries (test_string cunit)

add_executable (test_json test_json.c
    ../src/qiniu/base/errors.c
    ../src/qiniu/base/base64.c
    ../src/qiniu/base/string.c
    ../src/qiniu/base/json.c
    ../src/qiniu/base/json_parser.c
    ../src/qiniu/base/json_formatter.c)
target_link_libraries (test_json cunit)

add_executable (test_http_header test_http_header.c
    ../src/qiniu/base/ds/etable.c
    ../src/qiniu/base/errors.c
    ../src/qiniu/base/base64.c
    ../src/qiniu/base/string.c
    ../src/qiniu/http_header.c
    ../src/qiniu/http_header_parser.c)
target_link_libraries (test_http_header cunit)

add_executable (test_etag test_etag.c
    ../src/qiniu/base/errors.c
    ../src/qiniu/base/base64.c
    ../src/qiniu/base/string.c)
target_link_libraries (test_etag cunit crypto)
