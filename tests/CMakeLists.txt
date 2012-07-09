################################################################################
# Calcium Tests
################################################################################

project( TESTS )

# Add the tests 'modules' dir to the CORAL_PATH
set( CORAL_PATH "${TESTS_SOURCE_DIR}/modules" ${CORAL_PATH} )

add_subdirectory( lua )
add_custom_target( tests )

# Locate the GTest Framework. We can only add some tests if GTest is available.
find_package( GTest )
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

	add_subdirectory( src )
	add_subdirectory( dom1 )
	add_subdirectory( dom2 )

	add_dependencies( tests tests_core tests_lua tests_dom1 tests_dom2 )

	file( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bdEvoTest )
	file( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/output )
	add_test(
		NAME tests_dom1_pass1
		COMMAND ${CMAKE_COMMAND} chdir -E ${CMAKE_BINARY_DIR}/bdEvoTest
		$<TARGET_FILE:tests_dom1> --gtest_output=xml:../output/tests_dom1_pass1_$<CONFIGURATION>.xml --gtest_filter=*pass1*
	)
	CORAL_TEST_ENVIRONMENT( tests_dom1_pass1 )

	add_test(
		NAME tests_dom2
		COMMAND ${CMAKE_COMMAND} chdir -E ${CMAKE_BINARY_DIR}/bdEvoTest
		$<TARGET_FILE:tests_dom2> --gtest_output=xml:../output/tests_dom2_$<CONFIGURATION>.xml
	)
	CORAL_TEST_ENVIRONMENT( tests_dom2 )

	add_test(
		NAME tests_dom1_pass2
		COMMAND ${CMAKE_COMMAND} chdir -E ${CMAKE_BINARY_DIR}/bdEvoTest
		$<TARGET_FILE:tests_dom1> --gtest_output=xml:../output/tests_dom1_pass2_$<CONFIGURATION>.xml --gtest_filter=*pass2*
	)
	CORAL_TEST_ENVIRONMENT( tests_dom1_pass2 )

else()
	message( "WARNING: gtest not found - most tests have been disabled!" )
endif()
