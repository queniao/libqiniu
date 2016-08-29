add_executable (qdnurl qdnurl.c)
target_link_libraries (qdnurl qiniu crypto)

add_executable (qacctoken qacctoken.c)
target_link_libraries (qacctoken qiniu crypto)

add_executable (qstat qstat.c)
target_link_libraries (qstat qiniu curl crypto)

add_executable (qput qput.c)
target_link_libraries (qput qiniu curl crypto)
