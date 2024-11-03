cmake_minimum_required(VERSION 3.25)
project(FunVM VERSION 0.1.1)

set(FUNVM_COMMON "Common")
set(FUNVM_COMPILER "funvmc")
set(FUNVM_INTERPRETER "funvm")

# 32-bit architecture
set(CMAKE_C_FLAGS "-m32")

# All warnings and highest level of debugging output
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 -g3")

# supressed warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")

# Sanitizer
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -static-libasan -fno-omit-frame-pointer")


add_compile_definitions(
	FUNVM_ARCH_x64
)

add_subdirectory(vm)
add_subdirectory(compiler)
add_subdirectory(common)