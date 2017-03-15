cmake_minimum_required (VERSION 3.3.1)
project (libqiniu)

include (CheckSymbolExists)

set (QN_LARGE_FILE_SUPPORT_AWARE OFF CACHE BOOL "Set to ON to detect Large File Support dynamically (default: OFF)")
set (QN_SHARED_FD_FOR_SECTIONS_SUPPORT ON CACHE BOOL "Set to ON to enable shared fd for sections (default: ON)")

include_directories (/usr/include /usr/local/include SYSTEM)
link_directories (/usr/lib /usr/local/lib)

set (CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
check_symbol_exists (lseek64 "unistd.h" HAVE_LSEEK64)
check_symbol_exists (pread "unistd.h" HAVE_PREAD)
set (CMAKE_REQUIRED_DEFINITIONS "")

if (DEFINED HAVE_LSEEK64)
    add_compile_options (-DQN_CFG_LARGE_FILE_SUPPORT)
    if (DEFINED QN_LARGE_FILE_SUPPORT_AWARE AND ${QN_LARGE_FILE_SUPPORT_AWARE})
        add_compile_options (-DQN_CFG_LARGE_FILE_SUPPORT_AWARE)
    endif (DEFINED QN_LARGE_FILE_SUPPORT_AWARE AND ${QN_LARGE_FILE_SUPPORT_AWARE})
endif (DEFINED HAVE_LSEEK64)

if (DEFINED HAVE_PREAD AND DEFINED QN_SHARED_FD_FOR_SECTIONS_SUPPORT AND ${QN_SHARED_FD_FOR_SECTIONS_SUPPORT})
    add_compile_options (-DQN_CFG_SHARED_FD_FOR_SECTIONS)
endif (DEFINED HAVE_PREAD AND DEFINED QN_SHARED_FD_FOR_SECTIONS_SUPPORT AND ${QN_SHARED_FD_FOR_SECTIONS_SUPPORT})

add_compile_options (-D_GNU_SOURCE --std=c99 -Wall)

file (GLOB_RECURSE SOURCE_FILES src/qiniu/*.c)
include_directories (src)
add_library (qiniu SHARED ${SOURCE_FILES})

if (DEFINED HAVE_LSEEK64 AND DEFINED QN_LARGE_FILE_SUPPORT_AWARE AND ${QN_LARGE_FILE_SUPPORT_AWARE})
    target_link_libraries (qiniu dl)
endif (DEFINED HAVE_LSEEK64 AND DEFINED QN_LARGE_FILE_SUPPORT_AWARE AND ${QN_LARGE_FILE_SUPPORT_AWARE})

target_link_libraries (qiniu curl ssl crypto)

add_subdirectory (test)
add_subdirectory (demo)
#add_test (NAME test_json COMMAND test/test_json)
