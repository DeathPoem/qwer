cmake_minimum_required(VERSION 3.5)

configure_file("${MY_ROOT_PATH}/src/lib/config.h.in" "${MY_ROOT_PATH}/src/lib/config.h")
include_directories(${CMACK_CURRENT_BINARY_DIR})

file(GLOB MY_LIBRARRY_SOURCE "./lib/*.cpp")
file(GLOB THIRD_PARTY_SOURCE "../third_party/http-parser/*.cpp")
add_library(my_library ${MY_LIBRARRY_SOURCE} ${THIRD_PARTY_SOURCE})
#add_executable(raw_build_example raw_build_example.cpp)
#target_link_libraries(raw_build_example my_library)
