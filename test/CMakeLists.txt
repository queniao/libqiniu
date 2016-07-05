add_executable (test_string test_string.c ../src/base/string.c ../src/base/base64.c ../src/base/errors.c)
target_link_libraries (test_string cunit)

add_executable (test_json test_json.c ../src/base/json.c
    ../src/base/json_parser.c ../src/base/json_formatter.c
    ../src/base/string.c ../src/base/base64.c ../src/base/errors.c)
target_link_libraries (test_json cunit)
