#!/bin/bash

# For normal version

mkdir -p ./build

cd ./build/
CC=${LLVM_DIR}/bin/clang CXX=${LLVM_DIR}/bin/clang++ cmake ../instrumentationLib/
make -j4

${LLVM_DIR}/bin/llvm-link $(find . -name '*.o') -o instrumentation_lib.ll

echo "@ . @ Merged file is at: " $(realpath instrumentation_lib.ll)

# For ROS version


make clean

# CC=${LLVM_DIR}/bin/clang CXX=${LLVM_DIR}/bin/clang++ cmake -DROS_ENABLED=1 ../instrumentationLib/
CC=${LLVM_DIR}/bin/clang CXX=${LLVM_DIR}/bin/clang++ cmake ../instrumentationLib/
make -j4

${LLVM_DIR}/bin/llvm-link $(find . -name '*.o') -o ROS_enabled_instrumentation_lib.ll

echo "@ . @ Merged file is at: " $(realpath ROS_enabled_instrumentation_lib.ll)