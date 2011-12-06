################################################################################
# The 'tests' executable
################################################################################

project( TESTS )

include_directories(
	${GTEST_INCLUDE_DIRS}
	${ERM_BINARY_DIR}/generated
	${SERIALIZATION_BINARY_DIR}/generated
	${SERIALIZATION_SOURCE_DIR}
	${CA_BINARY_DIR}/generated
	${CA_SOURCE_DIR}
)

# Calcium sources replicated here for testing
set( REPLICATED_MODULE_FILES
	${CA_SOURCE_DIR}/sqlite/sqlite3.c
	${CA_SOURCE_DIR}/sqlite/StringSerializer.cpp
	${CA_SOURCE_DIR}/sqlite/SQLiteConnection.cpp
	${CA_SOURCE_DIR}/sqlite/SQLiteResultSet.cpp
)

# Gather source files in the current directory
file( GLOB _HEADERS *.h )
file( GLOB _SOURCES *.cpp persistence/*.cpp )

add_executable( tests EXCLUDE_FROM_ALL ${REPLICATED_MODULE_FILES} ${_HEADERS} ${_SOURCES} )

add_dependencies( tests camodels erm serialization )

CORAL_TARGET( tests )

set_target_properties( tests PROPERTIES
	PROJECT_LABEL "Tests"
)

target_link_libraries( tests ${GTEST_LIBRARIES} )

################################################################################
# Register the test
################################################################################
file( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/output )
add_test(
	NAME tests
	COMMAND $<TARGET_FILE:tests> --gtest_output=xml:../../output/Tests$<CONFIGURATION>.xml
)
CORAL_TEST_ENVIRONMENT( tests )

################################################################################
# If Valgrind is available, repeat the test checking for memory leaks
################################################################################
if( VALGRIND_COMMAND )
	add_test(
		NAME tests_MemoryCheck
		COMMAND ${VALGRIND_COMMAND} --leak-check=full --show-reachable=yes --num-callers=30 --dsymutil=yes
			--log-file=${CMAKE_BINARY_DIR}/Valgrind$<CONFIGURATION>.log --error-exitcode=13
			--suppressions=${CMAKE_SOURCE_DIR}/tests/valgrind.supp $<TARGET_FILE:tests>
	)
	CORAL_TEST_ENVIRONMENT( tests_MemoryCheck )
endif()

################################################################################
# Source Groups
################################################################################

source_group( "@Replicated" FILES ${REPLICATED_MODULE_FILES} )

file( GLOB miscFiles *.h *.cpp )
source_group( "Misc" FILES ${miscFiles} )

file( GLOB coreTestFiles *Tests.cpp )
source_group( "Core Tests" FILES ${coreTestFiles} )

file( GLOB persistenceTestFiles persistence/*Tests.cpp )
source_group( "Persistence Tests" FILES ${persistenceTestFiles} )
