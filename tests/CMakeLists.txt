# Locate the GTest Framework.
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

	add_subdirectory( src )
	add_subdirectory( dom1 )
	add_subdirectory( dom2 )

else()
	message( "WARNING: gtest not found - all tests have been disabled!" )
endif()
