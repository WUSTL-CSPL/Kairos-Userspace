cmake_minimum_required(VERSION 3.10)
project(TimingAnnomation VERSION 1.0)

# Set the C++ standard you are using
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add definitions to generate LLVM bitcode instead of regular binary
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")

# List your source files here
set(SOURCE_FILES
    handling-policy.cpp 
    timing-correctness.cpp 
    ROS-interface.cpp 
    handling-policy.h  
    timestamp-vector.h
    timing-correctness.h
    Shore-common.h
#### lib to communicate with kernel
${PROJECT_SOURCE_DIR}/../../Shore-Interface/interface-lib/Shore-user-interface-functions.cpp
${PROJECT_SOURCE_DIR}/../../Shore-Interface/interface-lib/Shore-user-interface-functions.h
)


# Create an executable from the source files
add_library(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/../../Shore-Interface/interface-lib)

# make bit code one file

