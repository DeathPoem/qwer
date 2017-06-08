#/bin/bash

#我们这么做是因为，在cmake里写死会导致ycmgenerator出问题
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
cmake ..
