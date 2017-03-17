cmake_minimum_required(VERSION 3.5)

add_library(my_library ./faci/facility.cpp)
add_executable(raw_build_example raw_build_example.cpp)
target_link_libraries(raw_build_example my_library)
