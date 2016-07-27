add_executable (qdnurl qdnurl.c)
target_link_libraries (qdnurl qiniu crypto)

add_executable (qacctoken qacctoken.c)
target_link_libraries (qacctoken qiniu crypto)
