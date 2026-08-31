cmake_minimum_required(VERSION 3.5)
project(extract-xiso)

string(TOUPPER __${CMAKE_SYSTEM_NAME}__ TARGET_OS)

set(SOURCE_FILES
	extract-xiso.c
)

if(WIN32)
	list(APPEND SOURCE_FILES
		win32/getopt.c
		win32/asprintf.c
	)
endif()

add_executable(extract-xiso ${SOURCE_FILES})

if(WIN32)
	target_include_directories(extract-xiso PRIVATE win32)
	target_compile_definitions(extract-xiso PRIVATE ${TARGET_OS} _CRT_SECURE_NO_WARNINGS WIN32)
endif()

install(TARGETS extract-xiso RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
