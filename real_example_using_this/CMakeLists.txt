cmake_minimum_required(VERSION 3.2)

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
set(2_QWER_ROOT "/qwer_main_softlink")
set(QWER_LIBRARY_ROOT ${online_example_SOURCE_DIR}/${2_QWER_ROOT})

#set(QWER_INCLUDES ${QWER_INCLUDES} 
#    ${QWER_LIBRARY_ROOT}/third_party/json/src 
#    ${QWER_LIBRARY_ROOT}/third_party/http-parser 
#    ${QWER_LIBRARY_ROOT}/third_party/SQLiteCpp/include 
#    ${QWER_LIBRARY_ROOT}/src 
#    ${QWER_LIBRARY_ROOT}/src/lib)
####
# qwer library
add_subdirectory(${QWER_LIBRARY_ROOT}) # would build my_library target, and third_party libraries in MY_EXTRA_LIBS target
message("add includes for qwer: ${QWER_INCLUDES}")
set(MY_EXTRA_LIBS ${QWER_LIBS})
include_directories(${QWER_INCLUDES})

find_package(Threads)
set(X_MY_EXTRA_LIBS ${MY_EXTRA_LIBS} ${CMAKE_THREAD_LIBS_INIT})   # use MY_EXTRA_LIBS target to build
message("qwer: ${QWER_LIBRARY_ROOT}")
message("extra library from qwer: ${MY_EXTRA_LIBS}")

###
# other library

###
# main daemon program

add_subdirectory(${online_example_SOURCE_DIR}/src)
