cmake_minimum_required(VERSION 3.5)

file(GLOB MY_LIBRARRY_SOURCE "./faci/*.cpp")
#set(MY_LIBRARRY_SOURCE "./faci/facility.cpp" "./faci/nettools.cpp")
add_library(my_library ${MY_LIBRARRY_SOURCE})
add_executable(raw_build_example raw_build_example.cpp)
target_link_libraries(raw_build_example my_library)
