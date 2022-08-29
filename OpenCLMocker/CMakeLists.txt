cmake_minimum_required (VERSION 3.8)

set (OpenCLMockerSrc
	src/API.cpp
	src/APIEnums.cpp
	src/Config.cpp
	src/Device.cpp
	src/Event.cpp
	src/Platform.cpp
	src/Kernel.cpp
	src/Queue.cpp
	src/Environment.cpp
	src/Buffer.cpp)

add_library(OpenCL SHARED ${OpenCLMockerSrc})
target_compile_features(OpenCL PRIVATE cxx_std_20)

target_link_libraries(OpenCL PRIVATE Threads::Threads)

target_include_directories(OpenCL
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/nlohmann/include/>
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include/>
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)

if (${CMAKE_BUILD_TYPE} EQUAL "Relase")
	target_compile_definitions(OpenCL
		PUBLIC OPENCL_CATCH_EXCEPTIONS)
endif()
