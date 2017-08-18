cmake_minimum_required(VERSION 3.5)

include_directories(${QWER_INCLUDES})

file(GLOB MYAPP "./*.cpp")
add_executable(myapp ${MYAPP})
message("link library for app: ${X_MY_EXTRA_LIBS}")
target_link_libraries(myapp ${X_MY_EXTRA_LIBS})
