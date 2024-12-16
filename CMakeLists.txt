cmake_minimum_required(VERSION 3.25)
project(FunVM VERSION 0.1.4)

set(FUNVM_COMMON "Common")
set(FUNVM_COMPILER "FVMCexe")
set(FUNVM_INTERPRETER "FVMexe")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) 

option(USE_DEBUGGER "Apply debugging flags" YES)
option(USE_SANITIZER "Apply memory leak and boundary violation control" NO)

if (USE_DEBUGGER)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Wall -O0 -g3")
	add_subdirectory(tests/native)
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Wall -Ofast")
endif()

if (USE_SANITIZER)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -static-libasan -fno-omit-frame-pointer")
endif()

add_compile_definitions(
	FUNVM_MEM_MANAGER
)

add_subdirectory(vm)
add_subdirectory(compiler)
add_subdirectory(common)