cmake_minimum_required(VERSION 3.12)

get_filename_component(projectName ${CMAKE_CURRENT_SOURCE_DIR} NAME)
message("cmake for ${projectName}")
project(${projectName})

set(CMAKE_CXX_STANDARD 20)

add_library (
    ${projectName}
    INTERFACE
)

target_include_directories(${projectName} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${projectName})










