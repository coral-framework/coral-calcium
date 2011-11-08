################################################################################
# The 'tests' executable
################################################################################

PROJECT( TESTS )

INCLUDE_DIRECTORIES(
	${GTEST_INCLUDE_DIRS}
	${ERM_BINARY_DIR}/generated
	${SERIALIZATION_BINARY_DIR}/generated
	${SERIALIZATION_SOURCE_DIR}
	${CMAKE_BINARY_DIR}/src/generated
	${CMAKE_SOURCE_DIR}/src
)

# Gather source files in the current directory
FILE( GLOB _HEADERS *.h )
FILE( GLOB _SOURCES *.cpp )

ADD_EXECUTABLE( tests EXCLUDE_FROM_ALL
	${_HEADERS}
	${_SOURCES}
	${_GENERATED_FILES}
	${CMAKE_SOURCE_DIR}/src/sqlite3.c
	${CMAKE_SOURCE_DIR}/src/StringSerializer.cpp	
	${CMAKE_SOURCE_DIR}/src/SQLiteDBConnection.cpp
	${CMAKE_SOURCE_DIR}/src/SQLiteResultSet.cpp
)

ADD_DEPENDENCIES( tests camodels erm serialization)

CORAL_TARGET( tests )

SET_TARGET_PROPERTIES( tests PROPERTIES
	PROJECT_LABEL "Tests"
)

TARGET_LINK_LIBRARIES( tests ${CORAL_LIBRARIES} ${GTEST_LIBRARIES} )

################################################################################
# Register the test
################################################################################
FILE( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/output )
ADD_TEST(
	NAME tests
	COMMAND $<TARGET_FILE:tests> --gtest_output=xml:../../output/Tests$<CONFIGURATION>.xml
)
CORAL_TEST_ENVIRONMENT( tests )

################################################################################
# If Valgrind is available, repeat the test checking for memory leaks
################################################################################
IF( VALGRIND_COMMAND )
	ADD_TEST(
		NAME tests_MemoryCheck
		COMMAND ${VALGRIND_COMMAND} --leak-check=full --show-reachable=yes --num-callers=30 --dsymutil=yes
			--log-file=${CMAKE_BINARY_DIR}/Valgrind$<CONFIGURATION>.log --error-exitcode=13
			--suppressions=${CMAKE_SOURCE_DIR}/tests/valgrind.supp $<TARGET_FILE:tests>
	)
	CORAL_TEST_ENVIRONMENT( tests_MemoryCheck )
ENDIF()

################################################################################
# Source Groups
################################################################################

SOURCE_GROUP( "@Generated" FILES ${_GENERATED_FILES} )
