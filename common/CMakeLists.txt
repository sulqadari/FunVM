add_library(${FUNVM_COMMON}
	bytecode.c
	const_pool.c
	memory.c
	object_pool.c
	object.c
	source_handler.c
	debug.c
	values.c
)

target_include_directories(${FUNVM_COMMON}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${PROJECT_SOURCE_DIR}/vm
)