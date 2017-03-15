add_executable (test_string test_string.c)
target_link_libraries (test_string qiniu cunit curl ssl crypto)

add_executable (test_json test_json.c)
target_link_libraries (test_json qiniu cunit curl ssl crypto)

add_executable (test_http_header test_http_header.c)
target_link_libraries (test_http_header qiniu cunit curl ssl crypto)

add_executable (test_etag test_etag.c)
target_link_libraries (test_etag qiniu cunit crypto curl ssl crypto)

add_executable (test_cdn test_cdn.c)
target_link_libraries (test_cdn qiniu cunit crypto curl ssl crypto)
