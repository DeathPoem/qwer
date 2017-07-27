cmake_minimum_required(VERSION 3.5)

include_directories(${online_example_SOURCE_DIR}/${QWER_LIBRARY_ROOT}/src/lib) # would include my_library 

file(GLOB MYAPP "./*.cpp")
add_executable(myapp ${MYAPP})
target_link_libraries(myapp ${my_extra_libs})
