################################################################################
# The 'tests' executable
################################################################################

PROJECT( TESTS )

CORAL_GENERATE_MAPPINGS( _GENERATED_FILES ca.ObjectModel co.System )

INCLUDE_DIRECTORIES( ${GTEST_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR}/generated )

# Gather source files in the current directory
FILE( GLOB _SOURCES *.cpp )

ADD_EXECUTABLE( tests EXCLUDE_FROM_ALL ${_SOURCES} ${_GENERATED_FILES} )
ADD_DEPENDENCIES( tests erm )

CORAL_DEFAULT_TARGET_PROPERTIES( tests )

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
	COMMAND $<TARGET_FILE:tests> --gtest_output=xml:../output/Tests$<CONFIGURATION>.xml
)

################################################################################
# Source Groups
################################################################################

SOURCE_GROUP( "@Generated" FILES ${_GENERATED_FILES} )
