include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/debug)

set(tests 
	test_op_constant_long test_vm)

foreach(test IN LISTS tests)
	add_executable("${test}" "${test}.c")
	target_link_libraries("${test}" funvm_debug funvm_sources)
	set_target_properties("${test}"
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
endforeach()