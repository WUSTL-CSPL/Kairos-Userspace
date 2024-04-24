set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
link_libraries(${JSONCPP_LIBRARIES})


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${Jsoncpp_INCLUDE_DIRS})

add_library(DataflowGraphBuilder SHARED PassDataflowGraphBuilder.cpp dependencyWalker.cpp)
target_link_libraries(DataflowGraphBuilder PRIVATE Svf) 


link_libraries(DataflowGraphBuilder)

set_target_properties(DataflowGraphBuilder PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

target_compile_features(DataflowGraphBuilder PRIVATE cxx_range_for cxx_auto_type)


set_target_properties(DataflowGraphBuilder PROPERTIES
	COMPILE_FLAGS "-fno-rtti"
)

