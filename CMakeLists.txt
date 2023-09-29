cmake_minimum_required(VERSION 3.25)
project(FunVM VERSION 0.2)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g2				\
	-Wall -Wextra -Wno-unused-parameter						\
	-Wno-missing-field-initializers -Wmissing-declarations	\
	-Wredundant-decls -Wcast-align							\
	-Wformat -Wformat-security"
)

option(FUNVM_DEBUG "Debug mode and unit tests" ON)

configure_file(funvmConfig.h.in funvmConfig.h)

add_subdirectory(src)

set(scriptFiles
	./test_scripts/functional/run.sh
	./test_scripts/functional/01_expressions.fn
	./test_scripts/functional/02_strings.fn
	./test_scripts/functional/03_globalVariables.fn
	./test_scripts/functional/04_localVariables.fn
	./test_scripts/functional/05_ifStatement.fn
	./test_scripts/functional/06_and.fn
	./test_scripts/functional/07_or.fn
	./test_scripts/functional/08_while.fn
	./test_scripts/functional/09_for.fn
	./test_scripts/functional/test_funcDecl.fn
	./test_scripts/functional/test_funcCall.fn
)

foreach(scriptFile IN LISTS scriptFiles)
	file(GENERATE 
		OUTPUT ${scriptFile}
		INPUT ${scriptFile}
	)
endforeach()

if(FUNVM_DEBUG)
	add_subdirectory(debug)
	add_subdirectory(tests)
	set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
	list(APPEND DEBUG_STUFF debug tests FUNVM_DEBUGGING)
endif()

add_executable(${CMAKE_PROJECT_NAME} main.c)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC FunVM_SRC ${DEBUG_STUFF})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_BINARY_DIR}/src")
