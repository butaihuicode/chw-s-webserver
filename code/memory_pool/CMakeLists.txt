aux_source_directory(. SRC_LIST)
add_library(MEMO STATIC ${SRC_LIST})
set(LIBRARY_OUTPUT_PATH ../lib)