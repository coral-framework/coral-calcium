################################################################################
# More tests for the Coral core
################################################################################

PROJECT( TESTS_DOM2 )

################################################################################
# Build the 'DOM2' module
################################################################################

SET( CORAL_PATH
	"${CORAL_ROOT}/modules"
	"${CMAKE_SOURCE_DIR}/modules"
    "${CMAKE_BINARY_DIR}/modules"
	"${CMAKE_BINARY_DIR}/modules/dom2"
    "${CMAKE_SOURCE_DIR}/tests/dom2"
)

CORAL_GENERATE_MODULE( _GENERATED_FILES dom )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}/generated )

FILE( GLOB _HEADERS *.h )
FILE( GLOB _SOURCES Company.cpp Employee.cpp Product.cpp Service.cpp )

ADD_LIBRARY( dom2 MODULE EXCLUDE_FROM_ALL ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )

CORAL_MODULE_TARGET( "dom2.dom" dom2 )

set_property( TARGET dom2 PROPERTY LIBRARY_OUTPUT_NAME dom )

SET_TARGET_PROPERTIES( dom2 PROPERTIES
	PROJECT_LABEL "Module DOM version 2"
)

################################################################################
# Build the test executable
################################################################################

# Pass the CORAL_PATH as a precompiler definition
CORAL_GET_PATH_STRING( CORAL_PATH_STR )

SET_PROPERTY( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS "CORAL_PATH=\"${CORAL_PATH_STR}\"" )

INCLUDE_DIRECTORIES( ${GTEST_INCLUDE_DIRS}
                     ${CA_BINARY_DIR}/generated
	                 ${CA_SOURCE_DIR}
					)

# Gather test source files in the current directory
FILE( GLOB _TESTS *Tests.cpp )

# Create test executable
ADD_EXECUTABLE( tests_dom2 EXCLUDE_FROM_ALL Main.cpp ${_TESTS} )
ADD_DEPENDENCIES( tests_dom2 dom2 ca )

CORAL_TARGET( tests_dom2 )

SET_TARGET_PROPERTIES( tests_dom2 PROPERTIES
	PROJECT_LABEL "Tests - DOM version 2"
)

TARGET_LINK_LIBRARIES( tests_dom2 ${GTEST_LIBRARIES} )

source_group( "@Generated" FILES ${_GENERATED_FILES} )

################################################################################
# Register the test
################################################################################

################################################################################
# If Valgrind is available, repeat the test checking for memory leaks
################################################################################
IF( VALGRIND_COMMAND )
	ADD_TEST(
		NAME tests_dom2_MemoryCheck
		COMMAND ${VALGRIND_COMMAND} --leak-check=full --show-reachable=yes --num-callers=15 --dsymutil=yes
		--log-file=${CMAKE_BINARY_DIR}/ValgrindDom2$<CONFIGURATION>.log --error-exitcode=13
		--suppressions=${CMAKE_SOURCE_DIR}/tests/valgrind.supp $<TARGET_FILE:tests_dom2>
	)
	CORAL_TEST_ENVIRONMENT( tests_dom2_MemoryCheck )
ENDIF()