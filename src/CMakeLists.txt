add_library(FunVM_SRC STATIC 
	bytecode.c
	compiler.c
	memory.c
	object.c
	scanner.c
	table.c
	utils.c
	value.c
	vm.c)

target_link_libraries(FunVM_SRC FUNVM_DEBUGGING)

# NOTE: 
# PUBLIC/INTERFACE modificator affects visibility.
# Need to find out the precise difference between them.
target_include_directories(FunVM_SRC
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${PROJECT_BINARY_DIR}
		${PROJECT_SOURCE_DIR}/debug)