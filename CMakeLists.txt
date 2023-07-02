cmake_minimum_required(VERSION 3.26.2)
project(FunVM VERSION 0.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast")

option(FUNVM_DEBUG "Debug mode and unit tests" OFF)
configure_file(funvmConfig.h.in funvmConfig.h)
add_subdirectory(src)

if(FUNVM_DEBUG)
	add_subdirectory(tests)
endif()

add_executable(FunVM main.c)
target_link_libraries(FunVM PUBLIC funvm_sources)
target_include_directories(FunVM PUBLIC
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include")