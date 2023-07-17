add_library(funvm_sources STATIC 
	bytecode.c
	compiler.c
	constant_pool.c
	memory.c
	scanner.c
	utils.c
	vm.c)

target_link_libraries(funvm_sources funvm_debug)

# NOTE: 
# PUBLIC/INTERFACE modificator affects visibility.
# Need to find out the precise difference between them.
target_include_directories(funvm_sources
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${PROJECT_BINARY_DIR}
		${PROJECT_SOURCE_DIR}/debug)