add_executable(heap_test
	heap_test.c
)

target_link_libraries(heap_test
	${FUNVM_COMMON}
)

target_include_directories(heap_test
	PRIVATE ${PROJECT_SOURCE_DIR}/common
)

add_executable(heap_realloc_test
	heap_realloc_test.c
)

target_link_libraries(heap_realloc_test
	${FUNVM_COMMON}
)

target_include_directories(heap_realloc_test
	PRIVATE ${PROJECT_SOURCE_DIR}/common
)