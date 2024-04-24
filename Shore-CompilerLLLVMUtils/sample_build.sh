#!/bin/bash

source ~/catkin_ws/devel/setup.bash
cd build

CC=${LLVM_DIR}/bin/clang CXX=${LLVM_DIR}/bin/clang++ cmake -DCMAKE_VERBOSE_MAKEFILE=ON ../amcl/


