cmake_minimum_required(VERSION 3.5)

get_filename_component(projectName EvalMaxSAT_bin NAME)

message("cmake for ${projectName}")

file(
        GLOB
	source_files
	main.cpp
)

set(localLibs
	MaLib
        EvalMaxSAT
        glucose
)

set(externLibs
        z
)

add_definitions(-DNDEBUG)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -O3")

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


FOREACH (localLib ${localLibs})
	install(DIRECTORY lib/${localLib}
		DESTINATION include
		FILES_MATCHING PATTERN "*.h")
ENDFOREACH(localLib)



