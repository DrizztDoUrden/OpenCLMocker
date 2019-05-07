cmake_minimum_required (VERSION 3.8)

set (OpenCLMockerSrc
	src/API.cpp
	src/Config.cpp
	src/Device.cpp
	src/Platform.cpp)

add_library(OpenCL SHARED ${OpenCLMockerSrc})
target_compile_features(OpenCL PRIVATE cxx_std_14)

target_include_directories(OpenCL
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include/>
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)

target_link_libraries(OpenCL PRIVATE nlohmann_json::nlohmann_json)
