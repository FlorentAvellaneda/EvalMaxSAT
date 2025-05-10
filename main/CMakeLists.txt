cmake_minimum_required(VERSION 3.12)

set(projectName EvalMaxSAT_bin)

message("cmake for ${projectName}")
project(${projectName})

set(CMAKE_CXX_STANDARD 17)

set(public_libs

)

set(private_libs
	MaLib
	cadical
	EvalMaxSAT
	z
)

file(
	GLOB_RECURSE
	source_files
	src/*
)

add_executable (
    ${projectName}
    ${source_files}
)

target_include_directories(${projectName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${projectName} PUBLIC ${public_libs} PRIVATE ${private_libs})

