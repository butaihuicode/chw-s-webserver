set(APP_SRC main.cpp)

add_subdirectory(http )
add_subdirectory(buffer ) 
add_subdirectory(epoller)
add_subdirectory(timer)
add_subdirectory(database)
add_subdirectory(webserver)

add_executable(${PROJECT_NAME} ${APP_SRC})
target_link_libraries(${PROJECT_NAME} WEBSERVER DATABASE HTTP TIMER BUFFER 
EPOLLER)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ROOT_DIR}/bin")