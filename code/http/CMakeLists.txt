aux_source_directory(. SRC_LIST)
add_library(HTTP STATIC ${SRC_LIST})
set(LIBRARY_OUTPUT_PATH ../lib)