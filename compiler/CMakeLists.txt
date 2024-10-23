add_executable(${FUNVM_COMPILER}
	compiler_main.c
	compiler.c
	scanner.c
	${PROJECT_SOURCE_DIR}/vm/vm.c
)

target_link_libraries(${FUNVM_COMPILER}
	${FUNVM_COMMON}
)

target_include_directories(${FUNVM_COMPILER}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PRIVATE ${PROJECT_SOURCE_DIR}/vm
)