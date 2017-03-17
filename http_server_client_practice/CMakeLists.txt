cmake_minimum_required(VERSION 3.5)
project(http_practice)
set(MY_ROOT_PATH http_practice_SOURCE_DIR)

###########
# config
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall --std=c++11 -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-g -Wall --std=c++11 -Wextra")
set(CMAKE_BUILD_TYPE DEBUG)

##########
# build
###
# json
include_directories(
    ${http_practice_SOURCE_DIR}/third_party/json/src
    )

include_directories(
    ${http_practice_SOURCE_DIR}/src
    ${http_practice_SOURCE_DIR}/src/faci
    )
###
# own stuff
add_subdirectory(src)
set(MY_EXTRA_LIBS ${MY_EXTRA_LIBS} my_library)

##########
# test
enable_testing()
add_subdirectory(third_party/googletest/googletest)
include_directories(
    ${gtest_SOURCE_DIR}/include
    )
add_subdirectory(test)

