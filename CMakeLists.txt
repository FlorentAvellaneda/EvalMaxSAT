cmake_minimum_required(VERSION 3.12)

set(projectName EvalMaxSAT_bin)

message("cmake for ${projectName}")
project(${projectName})

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


add_subdirectory(lib/MaLib)
add_subdirectory(lib/cadical)
add_subdirectory(lib/glucose)
add_subdirectory(lib/EvalMaxSAT)

add_subdirectory(main)

