add_library(funvm_debug STATIC debug.c)

include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/src)

target_link_libraries(funvm_debug funvm_sources)

# NOTE: 
# PUBLIC/INTERFACE modificator affects visibility.
# Need to find out the precise difference between them.
target_include_directories(funvm_debug
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/src)
