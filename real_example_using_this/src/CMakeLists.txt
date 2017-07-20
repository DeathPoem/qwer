cmake_minimum_required(VERSION 3.5)

include_directories(${CMACK_CURRENT_BINARY_DIR})

file(GLOB MYAPP "./*.cpp")
add_executable(myapp ${MYAPP})
