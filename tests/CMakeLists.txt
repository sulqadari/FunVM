include_directories(
	${PROJECT_BINARY_DIR}	
	${PROJECT_SOURCE_DIR}/include)

set(tests 
	test_simple_opcode_set)

foreach(test IN LISTS tests)
	add_executable("${test}" "${test}.c")
	target_link_libraries("${test}" funvm_sources)
	set_target_properties("${test}"
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
endforeach()