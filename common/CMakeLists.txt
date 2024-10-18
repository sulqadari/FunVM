add_library(${FUNVM_COMMON}
	source_handler.c
)

target_include_directories(${FUNVM_COMMON}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${PROJECT_SOURCE_DIR}/vm
)