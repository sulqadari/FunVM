add_library(funvm_debug debug.c)







target_include_directories(funvm_debug
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/src)
