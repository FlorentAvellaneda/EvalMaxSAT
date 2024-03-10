cmake_minimum_required(VERSION 3.12)

get_filename_component(projectName ${CMAKE_CURRENT_SOURCE_DIR} NAME)

message("cmake for ${projectName}")

project(${projectName})


# Spécifiez la version de C++ ici si nécessaire
set(CMAKE_CXX_STANDARD 20)

file(
	GLOB_RECURSE
	source_files
	src/*
)


add_library (
    MaLib
    ${source_files}
)

target_include_directories(MaLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)


# Liez MonSolveur avec MaLib
target_link_libraries(MaLib)

