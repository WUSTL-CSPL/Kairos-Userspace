# Installation



## Dependency Shore-Userspace Tools


Python dependencies

```
$ pip install pyquaternion seaborn numpy=1.20.3
```


### LLVM-13 toolchain

1. Install the dependency packages
    ```
    $ sudo apt-get install build-essential cmake ninja-build git python3
    ```


2. Clone the llvm project
    ```
    $ git clone https://github.com/llvm/llvm-project.git ~/llvm-project
    $ cd ~/llvm-project
    $ git checkout release/13.x
    ```

3. Compile the llvm project
    ```
    $ mkdir build
    $ cd build
    $ cmake -G "Unix Makefiles" \
        -DLLVM_ENABLE_PROJECTS="clang;lld" \
        -DLLVM_ENABLE_PLUGINS=ON \
        -DLLVM_BINUTILS_INCDIR=/usr/include \
        -DCMAKE_BUILD_TYPE=Release \
        ../llvm

    $ make make -j$(nproc)      # This may take a while
    ```

4. Export the llvm project path
    ```
    $ echo 'export LLVM_DIR=${HOME}/llvm-project/build' >> ~/.bashrc
    ```

### Haros
Steps to setup to configure the prerequisite for haros:

1. `$ cd ~/Shore-user/case-study/haros` 
and install the dependencies for haros:
`$ pip install -r requirements.txt`
2. Initialize haros:
python3 haros-runner.py init

A haros initial configuration file will be created at `~/.haros/configs.yaml`

Config the paths using the following example:
```
%YAML 1.1
---
workspace: '~/Shore-user/case-study/catkin_ws'
cpp:
   parser_lib: '/path/to/llvm_build/lib'
   std_includes: '/path/to/llvm_build/lib/clang/13.0.1/include'
   compile_db: '~/Shore-user/case-study/catkin_ws/build'
```



## Dependency for the Running Example -- ORB-SLAM3

More details instructions are in the original repository: 
https://github.com/UZ-SLAMLab/ORB_SLAM3

From our experience, two dependencies are required:

### Pangolin
Dowload and install instructions can be found at: https://github.com/stevenlovegrove/Pangolin.

### OpenCV

We tested with OpenCV 4.2. Here are the steps to install it.

Install the dependency packages:
```
$ sudo apt update
$ sudo apt install build-essential cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
$ sudo apt install libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libdc1394-22-dev
```

Download the OpenCV source code
```
$ git clone https://github.com/opencv/opencv.git
$ cd opencv
$ git checkout 4.2.0
$ cd ..

$ git clone https://github.com/opencv/opencv_contrib.git
$ cd opencv_contrib
$ git checkout 4.2.0
$ cd ..
```

Configure the build with cmake
```
$ cd opencv
$ mkdir build
$ cd build
$ cmake -D CMAKE_BUILD_TYPE=RELEASE \
      -D CMAKE_INSTALL_PREFIX=/usr/local \
      -D INSTALL_C_EXAMPLES=ON \
      -D INSTALL_PYTHON_EXAMPLES=ON \
      -D WITH_TBB=ON \
      -D WITH_V4L=ON \
      -D WITH_QT=ON \
      -D WITH_OPENGL=ON \
      -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
      -D BUILD_EXAMPLES=ON ..
```

Compile and install it

```
$ make -j$(nproc)
$ sudo make install
$ sudo ldconfig
```

Verify the installation

```
$ pkg-config --modversion opencv4
```

----

Now you finished the installation of dependency, go back to the Running Example in README.md TODO, give the link.
