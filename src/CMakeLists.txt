add_library(funvm_sources STATIC 
	memory.c bytecode.c constant_pool.c)

# NOTE: 
# PUBLIC/INTERFACE modificator affects visibility.
# Need to find out the precise difference between them.
target_include_directories(funvm_sources
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})