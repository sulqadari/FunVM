add_library(FUNVM_DEBUGGING STATIC debug.c)

include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/src)

target_link_libraries(FUNVM_DEBUGGING FunVM_SRC)

# NOTE: 
# PUBLIC/INTERFACE modificator affects visibility.
# Need to find out the precise difference between them.
target_include_directories(FUNVM_DEBUGGING
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/src)
