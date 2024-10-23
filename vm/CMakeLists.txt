add_executable(${FUNVM_INTERPRETER}
	vm_main.c
	vm.c
)

target_link_libraries(${FUNVM_INTERPRETER}
	${FUNVM_COMMON}
)

target_include_directories(${FUNVM_INTERPRETER}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${PROJECT_SOURCE_DIR}/common
)