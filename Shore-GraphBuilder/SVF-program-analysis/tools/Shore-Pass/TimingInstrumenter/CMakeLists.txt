set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

find_package(nlohmann_json 3.2.0 REQUIRED)


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

add_library(PassTimingInstrumenter SHARED PassTimingInstrumenter.cpp)
target_link_libraries(PassTimingInstrumenter PRIVATE Svf nlohmann_json::nlohmann_json) 


link_libraries(PassTimingInstrumenter)

set_target_properties(PassTimingInstrumenter PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

target_compile_features(PassTimingInstrumenter PRIVATE cxx_range_for cxx_auto_type)


set_target_properties(PassTimingInstrumenter PROPERTIES
	COMPILE_FLAGS "-fno-rtti"
)

