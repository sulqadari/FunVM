cmake_minimum_required(VERSION 3.25)
project(FunVM VERSION 0.2)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast")

option(FUNVM_DEBUG "Debug mode and unit tests" ON)
configure_file(funvmConfig.h.in funvmConfig.h)
file(GENERATE OUTPUT Arithmetic.fvm INPUT Arithmetic.fvm)
add_subdirectory(src)

if(FUNVM_DEBUG)
	add_subdirectory(debug)
	add_subdirectory(tests)
	set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
	list(APPEND DEBUG_STUFF debug tests funvm_debug)
endif()

add_executable(FunVM main.c)

target_link_libraries(FunVM PUBLIC funvm_sources ${DEBUG_STUFF})

target_include_directories(FunVM PUBLIC
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_BINARY_DIR}/src")