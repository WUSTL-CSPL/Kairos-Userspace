cmake_minimum_required(VERSION 2.8)
project(ORB_SLAM3)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

######
# Traditional method
######

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-flto")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
###############
# set(SHORE_DIR $ENV{SHORE_DIR})
# set(LLVM_CMAKE_PATH ${SHORE_DIR}/LLVM-compilation-tools)
# list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_PATH}/cmake")
# message(STATUS ${CMAKE_MODULE_PATH})
# include(LLVMIRUtil)
# ###############

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wall   -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall   -O0 -flto")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

# Check C++14 or C++0x support
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX1)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX14)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
   add_definitions(-DCOMPILEDWITHC11)
   message(STATUS "Using flag -std=c++14.")
elseif(COMPILER_SUPPORTS_CXX0X)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
   add_definitions(-DCOMPILEDWITHC0X)
   message(STATUS "Using flag -std=c++14.")
else()
   message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
endif()

LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules)

find_package(OpenCV 4)
   if(NOT OpenCV_FOUND)
      message(FATAL_ERROR "OpenCV > 4.4 not found.")
   endif()

MESSAGE("OPENCV VERSION:")
MESSAGE(${OpenCV_VERSION})

find_package(Eigen3 3.1.0 REQUIRED)
find_package(Pangolin REQUIRED)

include_directories(
${PROJECT_SOURCE_DIR}
${PROJECT_SOURCE_DIR}/include
${PROJECT_SOURCE_DIR}/include/CameraModels
${PROJECT_SOURCE_DIR}/Thirdparty/Sophus
${EIGEN3_INCLUDE_DIR}
${Pangolin_INCLUDE_DIRS}
)


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

add_library(${PROJECT_NAME} SHARED
src/System.cc
src/Tracking.cc
src/LocalMapping.cc
src/LoopClosing.cc
src/ORBextractor.cc
src/ORBmatcher.cc
src/FrameDrawer.cc
src/Converter.cc
src/MapPoint.cc
src/KeyFrame.cc
src/Atlas.cc
src/Map.cc
src/MapDrawer.cc
src/Optimizer.cc
src/Frame.cc
src/KeyFrameDatabase.cc
src/Sim3Solver.cc
src/Viewer.cc
src/ImuTypes.cc
src/G2oTypes.cc
src/CameraModels/Pinhole.cpp
src/CameraModels/KannalaBrandt8.cpp
src/OptimizableTypes.cpp
src/MLPnPsolver.cpp
src/GeometricTools.cc
src/TwoViewReconstruction.cc
src/Config.cc
src/Settings.cc
${PROJECT_SOURCE_DIR}/../../Shore-TimingAnnotation/annotationAPI/annotation-API.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../../Shore-TimingAnnotation/annotationAPI)

add_library(TimingAnnotationAPI SHARED
${PROJECT_SOURCE_DIR}/../../Shore-TimingAnnotation/annotationAPI/annotation-API.cpp
)

target_include_directories(TimingAnnotationAPI PUBLIC ${PROJECT_SOURCE_DIR}/../../Shore-TimingAnnotation/annotationAPI)

# # This is required if we compile it using Clang + LLVM
# set_target_properties(${PROJECT_NAME} PROPERTIES
# 	COMPILE_FLAGS "-fPIC"
# )


add_subdirectory(Thirdparty/g2o)

target_link_libraries(${PROJECT_NAME}
${OpenCV_LIBS}
${EIGEN3_LIBS}
${Pangolin_LIBRARIES}
${PROJECT_SOURCE_DIR}/Thirdparty/DBoW2/lib/libDBoW2.so
${PROJECT_SOURCE_DIR}/Thirdparty/g2o/lib/libg2o.so
-lboost_serialization
-lcrypto
)

##########################
# set(lambda_lib ${PROJECT_NAME})

# # Need to replace the name to the same as in the "add_executable"
# set_target_properties(${lambda_lib}
#     PROPERTIES
#     LINKER_LANGUAGE CXX
#     # ARCHIVE_OUTPUT_DIRECTORY "${EXE_DESTINATION}"
#     # LIBRARY_OUTPUT_DIRECTORY "${EXE_DESTINATION}"
#     # RUNTIME_OUTPUT_DIRECTORY "${EXE_DESTINATION}"
# )


# #### Generate bc
# llvmir_attach_bc_target(lambda_bc ${lambda_lib})
# add_dependencies(lambda_bc ${lambda_lib})


# #### Go through the pass
# # llvmir_attach_opt_pass_target(lambda_passed lambda_bc ${pass_options})
# # add_dependencies(lambda_passed lambda_bc)

# # message("After pass: ${lambda_passed}")

# ### Generate the lib

# llvmir_attach_library(${PROJECT_NAME}_transformed lambda_bc SHARED)
# add_dependencies(${PROJECT_NAME}_transformed lambda_bc)

# target_link_libraries(${PROJECT_NAME}_transformed
# ${OpenCV_LIBS}
# ${EIGEN3_LIBS}
# ${Pangolin_LIBRARIES}
# ${PROJECT_SOURCE_DIR}/Thirdparty/DBoW2/lib/libDBoW2.so
# ${PROJECT_SOURCE_DIR}/Thirdparty/g2o/lib/libg2o.so
# -lboost_serialization
# -lcrypto
# )

# #### Link the target executable
# # llvmir_attach_executable(lambda_pass_exe lambda_passed)

# set_target_properties(${PROJECT_NAME}_transformed PROPERTIES
# 	COMPILE_FLAGS "-fPIC"
# )

# # ## Enable compilation in default mode
# set_property(TARGET ${PROJECT_NAME}_transformed PROPERTY EXCLUDE_FROM_ALL OFF)
##########################


# # If RealSense SDK is found the library is added and its examples compiled
# if(realsense2_FOUND)
#     include_directories(${PROJECT_NAME}
#     ${realsense_INCLUDE_DIR}
#     )
#     target_link_libraries(${PROJECT_NAME}
#     ${realsense2_LIBRARY}
#     )
# endif()


# # Build examples

# # RGB-D examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/RGB-D)

# add_executable(rgbd_tum
#         Examples/RGB-D/rgbd_tum.cc)
# target_link_libraries(rgbd_tum ${PROJECT_NAME})

# if(realsense2_FOUND)
#     add_executable(rgbd_realsense_D435i
#             Examples/RGB-D/rgbd_realsense_D435i.cc)
#     target_link_libraries(rgbd_realsense_D435i ${PROJECT_NAME})
# endif()


# # RGB-D inertial examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/RGB-D-Inertial)

# #Stereo examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/Stereo)

# add_executable(stereo_kitti
#         Examples/Stereo/stereo_kitti.cc)
# target_link_libraries(stereo_kitti ${PROJECT_NAME})

# add_executable(stereo_euroc
#         Examples/Stereo/stereo_euroc.cc)
# target_link_libraries(stereo_euroc ${PROJECT_NAME})

# add_executable(stereo_tum_vi
#         Examples/Stereo/stereo_tum_vi.cc)
# target_link_libraries(stereo_tum_vi ${PROJECT_NAME})


# #Monocular examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/Monocular)

# add_executable(mono_tum
#         Examples/Monocular/mono_tum.cc)
# target_link_libraries(mono_tum ${PROJECT_NAME})

# add_executable(mono_kitti
#         Examples/Monocular/mono_kitti.cc)
# target_link_libraries(mono_kitti ${PROJECT_NAME})

# add_executable(mono_euroc
#         Examples/Monocular/mono_euroc.cc)
# target_link_libraries(mono_euroc ${PROJECT_NAME})

# add_executable(mono_tum_vi
#         Examples/Monocular/mono_tum_vi.cc)
# target_link_libraries(mono_tum_vi ${PROJECT_NAME})


# #Monocular inertial examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/Monocular-Inertial)

# add_executable(mono_inertial_euroc
#         Examples/Monocular-Inertial/mono_inertial_euroc.cc)
# target_link_libraries(mono_inertial_euroc ${PROJECT_NAME})

# add_executable(mono_inertial_tum_vi
#         Examples/Monocular-Inertial/mono_inertial_tum_vi.cc)
# target_link_libraries(mono_inertial_tum_vi ${PROJECT_NAME})



# #Stereo Inertial examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/Stereo-Inertial)

# add_executable(stereo_inertial_euroc
#         Examples/Stereo-Inertial/stereo_inertial_euroc.cc)
# target_link_libraries(stereo_inertial_euroc ${PROJECT_NAME})

# add_executable(stereo_inertial_tum_vi
#         Examples/Stereo-Inertial/stereo_inertial_tum_vi.cc)
# target_link_libraries(stereo_inertial_tum_vi ${PROJECT_NAME})



# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples/Calibration)


# #Old examples

# # RGB-D examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples_old/RGB-D)

# add_executable(rgbd_tum_old
#         Examples_old/RGB-D/rgbd_tum.cc)
# target_link_libraries(rgbd_tum_old ${PROJECT_NAME})




# # RGB-D inertial examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples_old/RGB-D-Inertial)



# #Stereo examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples_old/Stereo)

# add_executable(stereo_kitti_old
#         Examples_old/Stereo/stereo_kitti.cc)
# target_link_libraries(stereo_kitti_old ${PROJECT_NAME})

# add_executable(stereo_euroc_old
#         Examples_old/Stereo/stereo_euroc.cc)
# target_link_libraries(stereo_euroc_old ${PROJECT_NAME})

# add_executable(stereo_tum_vi_old
#         Examples_old/Stereo/stereo_tum_vi.cc)
# target_link_libraries(stereo_tum_vi_old ${PROJECT_NAME})



# #Monocular examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples_old/Monocular)

# add_executable(mono_tum_old
#         Examples_old/Monocular/mono_tum.cc)
# target_link_libraries(mono_tum_old ${PROJECT_NAME})

# add_executable(mono_kitti_old
#         Examples_old/Monocular/mono_kitti.cc)
# target_link_libraries(mono_kitti_old ${PROJECT_NAME})

# add_executable(mono_euroc_old
#         Examples_old/Monocular/mono_euroc.cc)
# target_link_libraries(mono_euroc_old ${PROJECT_NAME})

# add_executable(mono_tum_vi_old
#         Examples_old/Monocular/mono_tum_vi.cc)
# target_link_libraries(mono_tum_vi_old ${PROJECT_NAME})



# #Monocular inertial examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples_old/Monocular-Inertial)

# add_executable(mono_inertial_euroc_old
#         Examples_old/Monocular-Inertial/mono_inertial_euroc.cc)
# target_link_libraries(mono_inertial_euroc_old ${PROJECT_NAME})

# add_executable(mono_inertial_tum_vi_old
#         Examples_old/Monocular-Inertial/mono_inertial_tum_vi.cc)
# target_link_libraries(mono_inertial_tum_vi_old ${PROJECT_NAME})

# if(realsense2_FOUND)
#     add_executable(mono_inertial_realsense_t265_old
#             Examples_old/Monocular-Inertial/mono_inertial_realsense_t265.cc)
#     target_link_libraries(mono_inertial_realsense_t265_old ${PROJECT_NAME})

#     add_executable(mono_inertial_realsense_D435i_old
#             Examples_old/Monocular-Inertial/mono_inertial_realsense_D435i.cc)
#     target_link_libraries(mono_inertial_realsense_D435i_old ${PROJECT_NAME})
# endif()

# #Stereo Inertial examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Examples_old/Stereo-Inertial)

# add_executable(stereo_inertial_euroc_old
#         Examples_old/Stereo-Inertial/stereo_inertial_euroc.cc)
# target_link_libraries(stereo_inertial_euroc_old ${PROJECT_NAME})

# add_executable(stereo_inertial_tum_vi_old
#         Examples_old/Stereo-Inertial/stereo_inertial_tum_vi.cc)
# target_link_libraries(stereo_inertial_tum_vi_old ${PROJECT_NAME})

# if(realsense2_FOUND)
#     add_executable(stereo_inertial_realsense_t265_old
#             Examples_old/Stereo-Inertial/stereo_inertial_realsense_t265.cc)
#     target_link_libraries(stereo_inertial_realsense_t265_old ${PROJECT_NAME})

#     add_executable(stereo_inertial_realsense_D435i_old
#             Examples_old/Stereo-Inertial/stereo_inertial_realsense_D435i.cc)
#     target_link_libraries(stereo_inertial_realsense_D435i_old ${PROJECT_NAME})
# endif()
