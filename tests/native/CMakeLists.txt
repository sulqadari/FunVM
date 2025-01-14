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

add_executable(portable_serialize_test
	portable_create_test.c
)
target_link_libraries(portable_serialize_test
	${FUNVM_COMMON}
)
target_include_directories(portable_serialize_test
	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PRIVATE ${PROJECT_SOURCE_DIR}/vm
)

add_executable(portable_deserialize_test
	${CMAKE_CURRENT_SOURCE_DIR}/portable_read_test.c
)
target_link_libraries(portable_deserialize_test
	${FUNVM_COMMON}
)
target_include_directories(portable_deserialize_test
	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PRIVATE ${PROJECT_SOURCE_DIR}/vm
)