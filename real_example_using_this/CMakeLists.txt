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
set(QWER_LIBRARY_ROOT "/qwer_main_softlink")

###
# qwer library
add_subdirectory(${online_example_SOURCE_DIR}/${QWER_LIBRARY_ROOT}) # would build my_library target
set(my_extra_libs ${my_extra_libs} my_library)

###
# other library

###
# main daemon program

add_subdirectory(${online_example_SOURCE_DIR}/src)
