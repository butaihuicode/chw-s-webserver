
#out-of-source
cmake_minimum_required(VERSION 3.0)
project(HttpServer)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

#find_package(Jsoncpp)
set(ROOT_DIR ${PROJECT_SOURCE_DIR})

add_definitions(-Wall -g)
add_definitions(-L/usr/lib/aarch64-linux-gnu -lmysqlclient -lzstd -lssl -lcrypto -lresolv -lm)


add_subdirectory(code)




