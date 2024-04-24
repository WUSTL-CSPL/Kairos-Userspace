#!/bin/bash

current_dir=$(pwd)

## For ORB-SLAM Lib
echo "@-@ Compiling ORB-SLAM Lib"
cd ${current_dir}/ORB_SLAM3/
source ./build.sh


## For ROS node
echo "@-@ Compiling ORB-SLAM with ROS"
cd ${current_dir}/catkin_ws/
catkin_make clean
catkin_make --force-cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_CXX_COMPILER=${LLVM_DIR}/bin/clang++ -DCMAKE_C_COMPILER=${LLVM_DIR}/bin/clang
