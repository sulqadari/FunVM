add_library(funvm_sources STATIC 
		memory.c bytecode.c)

# NOTE: 
# When instead of PUBLIC I declared INTERFACE, the
# compilation ended up with error "cant find 'bytecode.h'"
# in compiling bytecode.c.
# Need to find out the precise difference between them.
target_include_directories(funvm_sources
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})