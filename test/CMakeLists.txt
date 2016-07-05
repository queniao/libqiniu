add_executable (test_string test_string.c ../src/qiniu/base/string.c
    ../src/qiniu/base/base64.c ../src/qiniu/base/errors.c)
target_link_libraries (test_string cunit)

add_executable (test_json test_json.c ../src/qiniu/base/json.c
    ../src/qiniu/base/json_parser.c ../src/qiniu/base/json_formatter.c
    ../src/qiniu/base/string.c ../src/qiniu/base/base64.c ../src/qiniu/base/errors.c)
target_link_libraries (test_json cunit)
