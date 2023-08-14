include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/debug)

set(tests 
	test_scanner)

foreach(test IN LISTS tests)
	add_executable("${test}" "${test}.c")
	target_link_libraries("${test}" FUNVM_DEBUGGING FunVM_SRC)
	set_target_properties("${test}"
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
endforeach()