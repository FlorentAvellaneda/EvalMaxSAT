cmake_minimum_required(VERSION 3.12)
set(projectName EvalMaxSAT_bin)
message("cmake for ${projectName}")
project(${projectName})

# List of local libraries
set(localLibs
	MaLib
    cadical
    EvalMaxSAT
)

# List of external libraries
set(externLibs
    z
)


set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Add local libraries
FOREACH(localLib ${localLibs})
    if(NOT TARGET ${localLib})
        add_subdirectory(lib/${localLib})
    endif()
ENDFOREACH(localLib)

find_package(ZLIB)
include_directories(${ZLIB_INCLUDE_DIR})

add_executable(${projectName} main.cpp)

FOREACH(localLib ${localLibs})
    target_include_directories(${projectName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/${localLib})
ENDFOREACH(localLib)

target_link_libraries(${projectName} 
    ${localLibs}
    ${externLibs}
)

install(TARGETS ${projectName}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
