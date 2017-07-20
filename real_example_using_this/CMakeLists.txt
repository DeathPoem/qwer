cmake_minimum_required(VERSION 3.5)

###########
# config
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "--std=c++14 -fpermissive -Wall -Wextra -lpthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-o2")
set(CMAKE_BUILD_TYPE DEBUG)

###########
# build
project(online_example)

###
# qwer library
include_directories(
    ${online_example_SOURCE_DIR}/../qwer_main/src/lib
    )
add_subdirectory(${online_example_SOURCE_DIR}/../qwer_main)
set(MY_EXTRA_LIBS ${MY_EXTRA_LIBS} my_library)

###
# other library

###
# main daemon program

add_subdirectory(${online_example_SOURCE_DIR}/src)
