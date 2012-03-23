#Locate the GTest Framework.
find_package( GTest )

# We can only add the test projects if GTest is available.
if( GTEST_FOUND )

	# On UNIX, GTest generally depends on pthreads, but CMake does not
	# add pthreads to GTEST_LIBRARIES. So we do it...
	if( UNIX )
		list( APPEND GTEST_LIBRARIES pthread )
	endif()

	# Check if Valgrind is available for the memory check tests
	find_program( VALGRIND_COMMAND valgrind )
	if( NOT VALGRIND_COMMAND )
		message( "WARNING: valgrind not found - all memory tests have been disabled!" )
	endif()

	set( CORAL_PATH ${CORAL_PATH}
		"${CORAL_ROOT}/modules"
	)
	
	add_subdirectory( src )
	add_subdirectory( dom1 )
	add_subdirectory( dom2 )

	add_custom_target( tests DEPENDS tests_core tests_dom1 tests_dom2 )
	
	file( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bdEvoTest )
	file( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/output )
	add_test(
		NAME tests_dom1_pass1
		COMMAND ${CMAKE_COMMAND} chdir -E ${CMAKE_BINARY_DIR}/bdEvoTest
		$<TARGET_FILE:tests_dom1> --gtest_output=xml:../output/TestsDom1Pass1$<CONFIGURATION>.xml --gtest_filter=*pass1*
	)
	CORAL_TEST_ENVIRONMENT( tests_dom1_pass1 )

	add_test(
		NAME tests_dom2
		COMMAND ${CMAKE_COMMAND} chdir -E ${CMAKE_BINARY_DIR}/bdEvoTest
		$<TARGET_FILE:tests_dom2> --gtest_output=xml:../output/TestsDom2$<CONFIGURATION>.xml
	)
	CORAL_TEST_ENVIRONMENT( tests_dom2 )
	
	add_test(
		NAME tests_dom1_pass2
		COMMAND ${CMAKE_COMMAND} chdir -E ${CMAKE_BINARY_DIR}/bdEvoTest
		$<TARGET_FILE:tests_dom1> --gtest_output=xml:../output/TestsDom1Pass2$<CONFIGURATION>.xml --gtest_filter=*pass2*
	)
	CORAL_TEST_ENVIRONMENT( tests_dom1_pass2 )
	
else()
	message( "WARNING: gtest not found - all tests have been disabled!" )
endif()
