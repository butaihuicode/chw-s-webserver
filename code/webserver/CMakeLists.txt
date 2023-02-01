aux_source_directory(. SRC_LIST)
add_library(WEBSERVER ${SRC_LIST} reactor.cpp proactor.h proactor.cpp)
set(LIBRARY_OUTPUT_PATH ../lib)