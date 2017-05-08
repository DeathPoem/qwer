cmake_minimum_required(VERSION 3.5)

project(unittest)
#################
# manually
###build test exe
##add_executable(simple_test simple_test.cpp)
##
### Standard linking to gtest stuff.
##target_link_libraries(simple_test gtest gtest_main)
##
### Extra linking for the project.
##target_link_libraries(simple_test ${MY_EXTRA_LIBS})
##################
# auto scan
# include_directories() call would only add include_dir to current CMakeLists.txt file
message("==== ENTRY ==== ${unittest_SOURCE_DIR}")
file(GLOB TESTFILES ./ *.cpp)
foreach(singletestfilename ${TESTFILES})
    get_filename_component(tn ${singletestfilename} NAME_WE)
    message("==== do with file target : ${tn}")
    add_executable(${tn} ${singletestfilename})
    target_link_libraries(${tn} gtest gtest_main)
    target_link_libraries(${tn} ${MY_EXTRA_LIBS})
    add_test(NAME case-${tn} 
        COMMAND ${tn})
endforeach(singletestfilename)
