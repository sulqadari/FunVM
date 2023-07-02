add_library(funvm_sources STATIC 
		memory.c bytecode.c)

#include_directories(${PROJECT_SOURCE_DIR}/include)

# NOTE: 
# When instead of PUBLIC I declared INTERFACE, the
# compilation ended up with error "cant find 'bytecode.h'"
# in compiling bytecode.c.
# Need to find out the preciese difference between them.
target_include_directories(funvm_sources
	PUBLIC "${PROJECT_SOURCE_DIR}/include")