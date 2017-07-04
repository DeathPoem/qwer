#/bin/bash

#我这么做是因为，在cmake里写死会导致ycmgenerator出问题
# Do this because I want using clang, not essential. Do this in this way because if I write it into CMakeLists.txt would let one of vim plugin crash.
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
cmake ..
