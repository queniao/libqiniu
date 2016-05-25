cmake_minimum_required (VERSION 3.5.2)
project (libqiniu)

#set(SOURCE_FILES src/base/base64.c src/base/string.c src/base/json.c)
include_directories (/usr/include /usr/local/include SYSTEM)
link_directories (/usr/lib /usr/local/lib)

file (GLOB_RECURSE SOURCE_FILES src/*.c)
include_directories (src)
add_library (qiniu STATIC ${SOURCE_FILES})

add_subdirectory (test)
add_test (NAME test_json COMMAND test/test_json)
