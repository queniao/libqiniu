add_executable (test_json test_json.c)
target_link_libraries (test_json qiniu cunit)

add_executable (test_string test_string.c)
target_link_libraries (test_string qiniu cunit)
