cmake_minimum_required(VERSION 2.8)

project(EvalMaxSAT)
get_filename_component(projectName EvalMaxSAT_bin NAME)

message("cmake for ${projectName}")

file(
        GLOB
	source_files
	main.cpp
        config.h
)

set(localLibs
	MaLib
        EvalMaxSAT
        cadical
)

set(externLibs
        z
        pthread
)


add_definitions(-DNDEBUG)
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -O3")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -O0 -g")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -O3 -g")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -O2 -static -lrt -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")


if(TARGET ${projectName})
	message("${projectName} is already defined")
else()
	FOREACH (localLib ${localLibs})
		add_subdirectory (lib/${localLib} ${localLib})
	ENDFOREACH(localLib)

	add_executable (
		${projectName}
		${source_files}
	)

	FOREACH (localLib ${localLibs})
		include_directories (lib/${localLib}/src)
	ENDFOREACH(localLib)

	set(LIB_LINK
		${localLibs}
		${externLibs}
	)

	target_link_libraries(${projectName}
	    ${LIB_LINK}
	)
endif()

install (TARGETS ${projectName} DESTINATION bin)

