#!/bin/bash
# This script changes to a specific directory and runs a command

# Specify the directory and command
current_dir=$(pwd)
target_directory="../catkin_ws"
command_to_run="catkin_make --force-cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_CXX_COMPILER=${LLVM_DIR}/bin/clang++ -DCMAKE_C_COMPILER=${LLVM_DIR}/bin/clang"

# Change to the target directory
cd "$target_directory"

# Check if the directory change was successful
if [ $? -eq 0 ]; then
    echo "Compiling orb_slam3_ros_wrapper..."
    # Run the command
    $command_to_run
    source devel/setup.bash
else
    echo "$target_directory not found"
    exit 1
fi

cd ~/Shore-user/case-study/catkin_ws/src/orb_slam3_ros_wrapper/haros_result

rm -rf results/
mkdir results/

python3 ~/Shore-user/case-study/haros/haros-runner.py --home /home/cspl/Shore-user/case-study/haros/haros_config analyse --no-cache -n -p orb_slam3.yaml -d results/

python3 ~/Shore-user/case-study/catkin_ws/src/orb_slam3_ros_wrapper/haros_result/haros_vis.py

cp ./haros_vis.pdf ${current_dir}/haros_vis.pdf

cd ${current_dir}

echo "Done"
