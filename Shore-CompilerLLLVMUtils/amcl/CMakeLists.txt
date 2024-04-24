cmake_minimum_required(VERSION 3.5)
project(amcl)

include(CheckIncludeFile)
include(CheckSymbolExists)

###############
set(SHORE_DIR $ENV{SHORE_DIR})
set(LLVM_CMAKE_PATH ${SHORE_DIR}/LLVM-compilation-tools)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_PATH}/cmake")
message(STATUS ${CMAKE_MODULE_PATH})
include(LLVMIRUtil)
###############

find_package(catkin REQUIRED
  COMPONENTS
    diagnostic_updater
    dynamic_reconfigure
    geometry_msgs
    message_filters
    nav_msgs
    rosbag
    roscpp
    sensor_msgs
    std_srvs
    tf2
    tf2_geometry_msgs
    tf2_msgs
    tf2_ros
)

find_package(Boost REQUIRED)

# dynamic reconfigure
generate_dynamic_reconfigure_options(
    cfg/AMCL.cfg
)

catkin_package(
  CATKIN_DEPENDS
    diagnostic_updater
    dynamic_reconfigure
    geometry_msgs
    nav_msgs
    rosbag
    roscpp
    sensor_msgs
    std_srvs
    tf2
    tf2_msgs
    tf2_ros
  INCLUDE_DIRS include
  LIBRARIES amcl_sensors amcl_map amcl_pf
)

include_directories(include)
include_directories(${catkin_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
include_directories(src/include)

check_include_file(unistd.h HAVE_UNISTD_H)
if (HAVE_UNISTD_H)
  add_definitions(-DHAVE_UNISTD_H)
endif (HAVE_UNISTD_H)

check_symbol_exists(drand48 stdlib.h HAVE_DRAND48)
if (HAVE_DRAND48)
  add_definitions(-DHAVE_DRAND48)
endif (HAVE_DRAND48)

add_library(amcl_pf
                    src/amcl/pf/pf.c
                    src/amcl/pf/pf_kdtree.c
                    src/amcl/pf/pf_pdf.c
                    src/amcl/pf/pf_vector.c
                    src/amcl/pf/eig3.c
                    src/amcl/pf/pf_draw.c)

add_library(amcl_map
                    src/amcl/map/map.c
                    src/amcl/map/map_cspace.cpp
                    src/amcl/map/map_range.c
                    src/amcl/map/map_store.c
                    src/amcl/map/map_draw.c)

add_library(amcl_sensors
                    src/amcl/sensors/amcl_sensor.cpp
                    src/amcl/sensors/amcl_odom.cpp
                    src/amcl/sensors/amcl_laser.cpp)
target_link_libraries(amcl_sensors amcl_map amcl_pf)

add_executable(amcl
                  src/amcl_node.cpp)
add_dependencies(amcl ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})


target_link_libraries(amcl
    amcl_sensors amcl_map amcl_pf
    ${Boost_LIBRARIES}
    ${catkin_LIBRARIES}
)



#####
# Ao Added
#####

# @CONFIGURE ME
set(LAMBDA_EXE amcl)
# Output path of the executable
set(EXE_DESTINATION /home/ao/catkin_ws/devel/lib/${LAMBDA_EXE})
# Pass path and configurations
set(pass_path /home/ao/Projects/llvm-tutor/build/lib/libHelloWorld.so)
set(pass_options -load-pass-plugin=${pass_path} -passes=hello-world)


# Need to replace the name to the same as in the "add_executable"
set_target_properties(${LAMBDA_EXE}
    PROPERTIES
    LINKER_LANGUAGE CXX
    # ARCHIVE_OUTPUT_DIRECTORY "${EXE_DESTINATION}"
    # LIBRARY_OUTPUT_DIRECTORY "${EXE_DESTINATION}"
    # RUNTIME_OUTPUT_DIRECTORY "${EXE_DESTINATION}"
)

#### Generate bc
llvmir_attach_bc_target(lambda_bc ${LAMBDA_EXE})
add_dependencies(lambda_bc ${LAMBDA_EXE})

#### Go through the pass
llvmir_attach_opt_pass_target(lambda_passed lambda_bc ${pass_options})
add_dependencies(lambda_passed lambda_bc)

message("After pass: ${lambda_passed}")

#### Link the target executable
llvmir_attach_executable(lambda_pass_exe lambda_passed)

# add_dependencies(lambda_pass_exe lambda_pass)
# target_link_libraries(lambda_pass_exe lambda_passed)

# set_target_properties(lambda_pass_exe
#     PROPERTIES
#     LINKER_LANGUAGE CXX
#     ARCHIVE_OUTPUT_DIRECTORY "${EXE_DESTINATION}"
#     LIBRARY_OUTPUT_DIRECTORY "${EXE_DESTINATION}"
#     RUNTIME_OUTPUT_DIRECTORY "${EXE_DESTINATION}"
# )

set_property(TARGET lambda_pass_exe PROPERTY EXCLUDE_FROM_ALL OFF)
######## End
