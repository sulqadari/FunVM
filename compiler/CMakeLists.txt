add_library(${CMAKE_PROJECT_NAME}_SRC STATIC 
	bytecode.c
	compiler.c
	memory.c
	object.c
	scanner.c
	table.c
	utils.c
	value.c
	vm.c)

if(FUNVM_DEBUG_VM)
	target_link_libraries(${CMAKE_PROJECT_NAME}_SRC ${CMAKE_PROJECT_NAME}_DEBUG)
endif()

# NOTE: 
# PUBLIC/INTERFACE modificator affects visibility.
# Need to find out the precise difference between them.
target_include_directories(${CMAKE_PROJECT_NAME}_SRC
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${PROJECT_BINARY_DIR}
		${PROJECT_SOURCE_DIR}/debug)