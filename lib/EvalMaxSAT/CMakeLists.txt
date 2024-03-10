cmake_minimum_required(VERSION 3.12)
get_filename_component(projectName ${CMAKE_CURRENT_SOURCE_DIR} NAME)
message("cmake for ${projectName}")
project(${projectName})

# List of local libraries
set(localLibs
	MaLib
    cadical
)

# List of external libraries
set(externLibs
    
)


set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

FOREACH(localLib ${localLibs})
    if(NOT TARGET ${localLib})
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../${localLib} ${CMAKE_CURRENT_BINARY_DIR}/${localLib})
    endif()
ENDFOREACH(localLib)

file(
	GLOB_RECURSE
	source_files
	src/*
)

add_library (
    ${projectName}
    ${source_files}
)

FOREACH(localLib ${localLibs})
    target_include_directories(${projectName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../${localLib})
ENDFOREACH(localLib)

target_include_directories(${projectName} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${projectName} 
    ${localLibs}
    ${externLibs}
)

install(TARGETS ${projectName}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)