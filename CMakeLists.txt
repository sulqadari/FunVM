cmake_minimum_required(VERSION 3.25)
project(FunVM VERSION 0.0.0)

set(FUNVM_COMMON "Common")
set(FUNVM_COMPILER "funvmc")
set(FUNVM_INTERPRETER "funvm")

# All warnings and highest level of debugging output
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 -g3")
# Sanitizer
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -static-libasan -fno-omit-frame-pointer")
# Compile the 32-bit version
set(CMAKE_C_FLASG "${CMAKE_C_FLAGS} -m32")

add_compile_definitions(
	FUNVM_ARCH_x64
)

add_subdirectory(vm)
add_subdirectory(compiler)
add_subdirectory(common)