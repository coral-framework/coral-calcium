################################################################################
# The 'tests' executable
################################################################################

project( TESTS_CORE )

include_directories(
	${GTEST_INCLUDE_DIRS}
	${ERM_BINARY_DIR}/generated
	${GRAPH_BINARY_DIR}/generated
	${CAMODELS_BINARY_DIR}/generated
	${SERIALIZATION_BINARY_DIR}/generated
	${SERIALIZATION_SOURCE_DIR}
	${CA_BINARY_DIR}/generated
	${CA_SOURCE_DIR}
)

# Calcium sources replicated here for testing
set( REPLICATED_MODULE_FILES
	${CA_SOURCE_DIR}/persistence/StringSerializer.cpp
	${CA_SOURCE_DIR}/persistence/sqlite/sqlite3.c
	${CA_SOURCE_DIR}/persistence/sqlite/SQLite.cpp
	${CA_SOURCE_DIR}/persistence/SpaceUpdater.cpp
)

# Gather source files in the current directory
file( GLOB _HEADERS *.h )
file( GLOB _SOURCES *.cpp persistence/*.cpp )

add_executable( tests_core EXCLUDE_FROM_ALL ${REPLICATED_MODULE_FILES} ${_HEADERS} ${_SOURCES} )

add_dependencies( tests_core camodels erm graph serialization )

CORAL_TARGET( tests_core )

set_target_properties( tests_core PROPERTIES
	PROJECT_LABEL "Tests Core"
)

target_link_libraries( tests_core ${GTEST_LIBRARIES} )

################################################################################
# Register the test
################################################################################

file( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/output )
add_test(
	NAME tests_core
	COMMAND $<TARGET_FILE:tests_core> --gtest_output=xml:../../output/tests_core_$<CONFIGURATION>.xml
)
CORAL_TEST_ENVIRONMENT( tests_core )

################################################################################
# If Valgrind is available, repeat the test checking for memory leaks
################################################################################
if( VALGRIND_COMMAND )
	add_test(
		NAME tests_core_MemoryCheck
		COMMAND ${VALGRIND_COMMAND} --leak-check=full --show-reachable=yes --num-callers=30 --dsymutil=yes
			--log-file=${CMAKE_BINARY_DIR}/Valgrind$<CONFIGURATION>.log --error-exitcode=13
			--suppressions=${CMAKE_SOURCE_DIR}/tests/valgrind.supp $<TARGET_FILE:tests_core>
	)
	CORAL_TEST_ENVIRONMENT( tests_core_MemoryCheck )
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
