cmake_minimum_required(VERSION 3.25)
project(FunVM VERSION 0.11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g			\
	-Wextra -Wno-unused-parameter						\
	-Wno-missing-field-initializers -Wmissing-declarations	\
	-Wredundant-decls -Wcast-align							\
	-Wformat -Wformat-security"
)

option(FUNVM_DEBUG_VM "Debug mode and unit tests" OFF)
option(FUNVM_DEBUG_GC "Garbage Collector's logger" OFF)
option(FUNVM_DEBUG_GC_STRESS "Run GC as often, as possible" OFF)

configure_file(funvmConfig.h.in funvmConfig.h)

add_subdirectory(src)

if(FUNVM_DEBUG_VM)
	add_subdirectory(debug)
	add_subdirectory(tests/native)
	set(CMAKE_C_FLAGS_DEBUG "-g2 -DDEBUG")
	list(APPEND DEBUG_STUFF debug tests ${CMAKE_PROJECT_NAME}_DEBUG)
endif()

add_executable(${CMAKE_PROJECT_NAME} main.c)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_PROJECT_NAME}_SRC ${DEBUG_STUFF})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_BINARY_DIR}/src")
