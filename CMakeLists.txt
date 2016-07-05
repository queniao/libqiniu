cmake_minimum_required (VERSION 3.3.1)
project (libqiniu)

include_directories (/usr/include /usr/local/include SYSTEM)
link_directories (/usr/lib /usr/local/lib)

file (GLOB_RECURSE SOURCE_FILES src/qiniu/*.c)
include_directories (src)
add_library (qiniu STATIC ${SOURCE_FILES})

list (APPEND CMAKE_C_FLAGS "--std=c99 -Wall")

add_subdirectory (test)
#add_test (NAME test_json COMMAND test/test_json)
