cmake_minimum_required(VERSION 3.12)

get_filename_component(projectName ${CMAKE_CURRENT_SOURCE_DIR} NAME)
message("cmake for ${projectName}")
project(${projectName})

set(CMAKE_CXX_STANDARD 20)

set(public_libs

)

set(private_libs
m
z
)

file(
	GLOB_RECURSE
	source_files
	src/*
)

add_library (
    ${projectName}
    ${source_files}
)

target_include_directories(${projectName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${projectName} PUBLIC ${public_libs} PRIVATE ${private_libs})
