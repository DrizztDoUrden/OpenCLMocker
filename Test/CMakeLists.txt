cmake_minimum_required (VERSION 3.8)

add_executable (Test "Test.cpp")
target_compile_features(Test PRIVATE cxx_std_14)

target_link_libraries(Test OpenCL)
