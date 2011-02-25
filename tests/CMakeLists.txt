# Locate the GTest Framework.
FIND_PACKAGE( GTest )

# We can only add the test projects if GTest is available.
IF( GTEST_FOUND )

	# On UNIX, GTest generally depends on pthreads, but CMake does not
	# add pthreads to GTEST_LIBRARIES. So we do it...
	IF( UNIX )
		LIST( APPEND GTEST_LIBRARIES pthread )
	ENDIF()

	# Check if Valgrind is available for the memory check tests
	FIND_PROGRAM( VALGRIND_COMMAND valgrind )
	IF( NOT VALGRIND_COMMAND )
		MESSAGE( "WARNING: valgrind not found - all memory tests have been disabled!" )
	ENDIF()

	ADD_SUBDIRECTORY( src )

ELSE()
	MESSAGE( "WARNING: gtest not found - all tests have been disabled!" )
ENDIF()
