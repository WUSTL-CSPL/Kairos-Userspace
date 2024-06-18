# OSDI 2024 --  Data-flow Availability: Achieving Timing Assurance in Autonomous Systems

## Introduction

This document gives the instructions for evaluating the functionality in the OSDI 2024 paper, "Data-flow Availability: Achieving Timing Assurance in Autonomous Systems."




## Overview
The proof-of-concept system of our paper is called Shore. It primarily consists of three GitHub repositories.

| Project name    | Github Link | Description |
|-----------------|-------------|-------------|
| Shore-Userspace | [Link](https://github.com/WUSTL-CSPL/Shore-Userspace)        |     This repository contains the Shore's compiler passes and tools used in user space.         |
| Shore-Middlware | [Link](https://github.com/WUSTL-CSPL/Shore-Middleware)        | Runtime enforcement of scheduling in ROS middleware.            |
| Shore-Kernel    | [Link](https://github.com/WUSTL-CSPL/Shore-Kernel)        |     Runtime enforcement of CPU scheduling and network packet scheduling in the Linux kernel.        |

We also open source the timing issues we collected and studied in Motivation section: :point_right: [Link](todo)
### Source Code Layout 

* **Shore-User**:
    - `/Shore-CompilerLLLVMUtils`: CMake scripts for automating the compilation via LLVM passes.
    - `/Shore-GraphBuilder`: Files for building _Timing Dependency Graph_.
        - `/ROS-depenedency-analyzer`: Python scripts to analyze the dependencies in ROS applications.
    - `/Shore-TimingAnnotation`: Files for annotating timing correctness properties
        - `/handling-policy`: The files for defining handling policies.
    - `/Shore-Interface`: Kernel module that handles the scheduling decision from user-space. 
    - `/ORB-SLAM3`: Running example, ORB-SLAM, downloaded from this [link](https://github.com/UZ-SLAMLab/ORB_SLAM3).
* **Shore-Kernel**:  
    - The modifications are mainly located in `kernel/sched` and `net/sched`. Most added functions and structs are marked by prefix `shore_`.
* **Shore-Middleware**: . . 
    - Most modifications are in the `ros_comm/roscpp/` folder. These modifications primarily involve:
        - Integrating additional logic into the vanilla scheduler.
        - Passing shoreline information to associate it with user-level tasks and kernel tasks.


## Getting Started

**Build from Source Code on Your Local Machine**

Please go the guided of installation:

Shore-Userspace: :point_right: [link](https://github.com/WUSTL-CSPL/Shore-Userspace/blob/main/INSTALLATION.md)  
Shore-Middleware: :point_right:  [link](https://github.com/WUSTL-CSPL/Shore-Middleware/blob/main/README.md)  
Shore-Kernel: :point_right:  [link](https://github.com/WUSTL-CSPL/Shore-Kernel/blob/main/README.MD)

## A Running Example

We use ORB-SLAM3 as the running example to demonstrate the functionality. ORB-SLAM3 is also the running example utilized in our submission paper (Section III).


### 1. Visual the Program's Task Data Dependency [Optional]

Steps:

1. Go to the haros analyzer directory:
```
$ cd ~/Shore-user/case-study/haros
```
2. Run the visualization script using the following command:
```
$ source ./vis_script.sh
```
3. The script will cp the generated result `haros_vis.pdf` to `~/Shore-user/case-study/haros`.


<img src="./scripts/vis.pdf" alt="task-data-dependency" width="700" height="400">



The figure displays the task names and their data dependencies, indicated by arrows. It also reports the **file name** and **line number** of the tasks in the source code, which can guide users in annotating the timing constraints.


### 2. Compile the instrumentation-related libs/tools

Compile the LLVM pass
```
$ cd ~/Shore-user/Shore-GraphBuilder/SVF-program-analysis
$ ./build.sh
```

Compile the libs requirement by instrumentation

```
$ cd ~/Shore-user/Shore-TimingAnnotation/
$ ./build_instrumentationLib.sh
```

### 3. Annotate the Timing Properties/Constraints

User can annotate the timing property via Shore's API (i.e., FRESHNESS(), CONSISTENCY(), and STABILITY()). 
The annotation might require some understanding of the target program, as discussed in the paper. We have already annotated two types of constraints in the source code:
- STABILITY() in the file `ORB_SLAM3/src/LocalMapping.cc`
- CONSISTENCY() in the file `case-study/catkin_ws/src/orb_slam3_ros_wrapper/src/stereo_inertial_node.cc`

Reviewers can modify the timing properties or annotate new constraints.

### 4. Compile and Instrument the Program to Enable DFA

Compile ORB-SLAM3 library and its ROS executable
```
$ cd ~/Shore-user/case-study
$ ./compile_orb_slam_with_ros.sh 
```
Instrument the program via LLVM pass
```
$ ./instrument_orbslam_with_pass.sh
```

### 5. Launch the Kernel Module for User-Kernel Communication
This step allows the target program to communicate with kernel schedulers, including task and network packet schedulers. If the sudo password is required, it is `cspl`.

```
$ cd ~/Shore-user/Shore-Interface/kernel-interface-module
### The usage is sudo ./shore_kernel_support_script.sh <enable|disable>
$ sudo ./shore_kernel_support_script.sh enable
```

Note: if it report an error, probably the interface module is already enabled.  

### 6. Inject System Overhead to Emulate Timing Anormaly [Optional]

This steps aims to emulate the high system overhead.

Install the aggressor workload tool __stress-ng__ if necessary
`$ sudo apt-get install stress-ng`

Launch the aggressor workload to inject CPU overhead, and then proceed to the next step without waiting for it to finish.
```
$ cd ~/Shore-user/Shore-Interface
$ ./inject-CPU-overhead.sh
Enter desired CPU load percentage (1-100): # enter 60 here. 
```



### 7. Run ORB-SLAM and Obtain the Results

**1. Run ORB-SLAM3:**

Open three separate terminals and then  
  
  Terminal 1: 
  ```
  $ pkill roscore      # Kill rosmaster if it is already running 
  $ roscore
  ```  
  Terminal 2: 
  ```
  $ source ~/Shore-user/case-study/catkin_ws/devel/setup.bash
  $ roslaunch orb_slam3_ros_wrapper euroc_stereoimu.launch
  ```  

Wait until the terminal displays the log: `[Shore-Middleware] User-level task is registered`.
  
  Terminal 3: 
  ```
  $ cd ~/Shore-user/case-study/
  $ rosbag play MH_03_medium.bag
  ```  


Output Logs: Some logs are tagged with [Shore-debug] and may be outputted. These logs include default timestamps, usage timestamps, and anomaly detection details. For example:

```
[Shore-Middlware] User-level task is registered

Vertex ID: 3
Current Def Timestamp is : 1403637258.488319 ｜ 
Current Use Timestamp is : 1713996090.786169 
------------------------------------
------------------------------------
Vertex ID: 4
Current Def Timestamp is : 1403637258.393319 ｜ 1403637258.338319 ｜ 1403637258.338319 ｜ 
Current Use Timestamp is : 1713996090.688732

```
**Note**: The large difference between the def and use times is due to the def time being from sensor data replayed from a previously recorded ROS bag.


Warning logs originating from the native ORB-SLAM3 software are normal. Such as the following:
```
not IMU meas

Not enough motion for initializing. Reseting...

LM: Reseting current map in Local Mapping...

not enough acceleration
```


**2. Get the results of ORB-SLAM**:
Once a run is complete (specifically, when the `rosbag play` program in Terminal 3 has finished), terminate the program in Terminal 2 by pressing `Ctrl+C`. The program will terminate and automatically log the trajectory file `~/.ros/FrameTrajectory_TUM_Format.txt`, which contains the algorithmic results of ORB-SLAM3.


### 8. Visualize the Results

Once a run is complete, copy the output file `FrameTrajectory_TUM_Format.txt` to the appropriate folder: 
```
$ mv ~/.ros/FrameTrajectory_TUM_Format.txt ~/Shore-user/case-study/plots/data/dfa.txt
```

Visualize the trajectories

```
$ cd ~/Shore-user/case-study/plots
$ python3 vis_orb_slam_traj.py
```
This will produce the figure `dfa_on_orbslam.pdf` (Figure 4(a) in the paper).

Visualize the translational errors in localization over time
```
$ python3 vis_orb_slam_traj_errors.py
```
This will reproduce the figure `orb-slam-errors.pdf` (Figure 4(b) in the paper).
      

### 9. [Optional] Generating Other Results

The steps outlined above generate results for ORB-SLAM under high system overhead with Shore mitigation. Users can also follow these steps to generate results in `~/Shore-user/case-study/plots/data`:

(i) **baseline.txt** - without system overhead. This can be achieved by commenting out:
- `STABILITY()` in the file `~/Shore-user/case-study/ORB_SLAM3/src/LocalMapping.cc`.
- `CONSISTENCY()` in the file `~/Shore-user/case-study/catkin_ws/src/orb_slam3_ros_wrapper/src/stereo_inertial_node.cc` within the ORB_SLAM3 source code. Then, recompile the project as per Step 3 and run it as outlined in Step 6.

(ii) **abnormal.txt** - under high system overhead but without the Shore mitigation. This requires:
- Commenting out also`STABILITY()` in the file `~/Shore-user/case-study/ORB_SLAM3/src/LocalMapping.cc`.
- Commenting out also `CONSISTENCY()` in the file `~/Shore-user/case-study/catkin_ws/src/orb_slam3_ros_wrapper/src/stereo_inertial_node.cc` within the ORB_SLAM3 source code. Then, recompile the project as per Step 3, inject overhead as described in Step 5, and run it as outlined in Step 6. 
