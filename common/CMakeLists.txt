add_library(${FUNVM_COMMON}
	bytecode.c
	const_pool.c
	memory.c
	value.c
	object.c
	object_pool.c
	hash_table.c
	globals.c
	heap.c
)

target_include_directories(${FUNVM_COMMON}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${PROJECT_SOURCE_DIR}/vm
)