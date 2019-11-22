cmake_minimum_required(VERSION 3.13)

project(SimpleAsmJitExample)

set (CMAKE_CXX_STANDARD 11)
include("asmjit/CMakeLists.txt")

#include_Directories(${ASMJIT_DIR})

# Then in your target you should be able to use:
add_executable(my_test main.cpp)
#target_link_libraries(my_test asmjit ${ASMJIT_DEPS})
target_link_libraries(my_test asmjit)


