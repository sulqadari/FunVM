add_library(${CMAKE_PROJECT_NAME}_DEBUG STATIC debug.c)

include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/src)

target_link_libraries(${CMAKE_PROJECT_NAME}_DEBUG ${CMAKE_PROJECT_NAME}_SRC)

# NOTE: 
# PUBLIC/INTERFACE modificator affects visibility.
# Need to find out the precise difference between them.
target_include_directories(${CMAKE_PROJECT_NAME}_DEBUG
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/src)
