cmake_minimum_required(VERSION 3.25)
project(FunVM VERSION 0.11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g2				\
	-Wall -Wextra -Wno-unused-parameter						\
	-Wno-missing-field-initializers -Wmissing-declarations	\
	-Wredundant-decls -Wcast-align							\
	-Wformat -Wformat-security"
)

option(FUNVM_DEBUG_VM "Debug mode and unit tests" OFF)

configure_file(funvmConfig.h.in funvmConfig.h)

add_subdirectory(src)

if(FUNVM_DEBUG_VM)
	add_subdirectory(debug)
	add_subdirectory(tests/native)
	set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
	list(APPEND DEBUG_STUFF debug tests FUNVM_DEBUGGING)
endif()

add_executable(${CMAKE_PROJECT_NAME} main.c)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC FunVM_SRC ${DEBUG_STUFF})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_BINARY_DIR}/src")
