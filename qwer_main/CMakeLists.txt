cmake_minimum_required(VERSION 3.5)

###########
# config
#set(CMAKE_C_COMPILER "/usr/bin/clang")         # to use YCMconfig, we need pass clang into cmake by a script
#set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "--std=c++14 -fpermissive -Wall -Wextra -lpthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-o2")
set(CMAKE_BUILD_TYPE DEBUG)

##########
# build
project(http_practice)
set(MY_ROOT_PATH ${http_practice_SOURCE_DIR})
###
# json
include_directories(
    ${http_practice_SOURCE_DIR}/third_party/json/src
    )

###
# own stuff
include_directories(
    ${http_practice_SOURCE_DIR}/src
    ${http_practice_SOURCE_DIR}/src/lib
    )
add_subdirectory(src)
set(MY_EXTRA_LIBS ${MY_EXTRA_LIBS} my_library)

##########
# test
enable_testing()
if (CMAKE_CONFIGURATION_TYPES)
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --output-on-failure
        --build-config "$<CONFIGURATION>")
else ()
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --output-on-failure)
endif()
add_subdirectory(third_party/googletest/googletest)
include_directories(
    ${gtest_SOURCE_DIR}/include
    )
add_subdirectory(test)
