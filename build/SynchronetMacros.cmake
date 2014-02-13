# Supress warnings about "deprecated POSIX function names"
# (Some aren't POSIX, and none are deprecated by POSIX)
if(MSVC)
	set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS /wd4996)
endif()

macro(double_require_lib_dir TARGET LIB LIBDIR)
	if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
		if(NOT DEFINED ${LIBDIR}_DONE)
			add_subdirectory(../../${LIBDIR} ${LIB})
			set(${LIBDIR}_DONE TRUE)
		endif()
	endif()
	target_include_directories(${TARGET} PRIVATE ../../${LIBDIR})
	target_link_libraries(${TARGET} ${LIB})
endmacro()

macro(double_require_lib TARGET LIB)
	double_require_lib_dir(${TARGET} ${LIB} ${LIB})
endmacro()

macro(require_lib_dir TARGET LIB LIBDIR)
	if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
		if(NOT DEFINED ${LIBDIR}_DONE)
			add_subdirectory(../${LIBDIR} ${LIB})
			set(${LIBDIR}_DONE TRUE)
		endif()
	endif()
	target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../${LIBDIR})
	target_link_libraries(${TARGET} ${LIB})
endmacro()

macro(require_lib TARGET LIB)
	require_lib_dir(${TARGET} ${LIB} ${LIB})
endmacro()

