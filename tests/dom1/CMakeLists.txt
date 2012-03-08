################################################################################
# More tests for the Coral core
################################################################################

PROJECT( TESTS_DOM1 )

################################################################################
# Build the 'DOM1' module
################################################################################


SET( CORAL_PATH
	"${CORAL_ROOT}/modules"
	"${CORAL_ROOT}/lib"
	"${CMAKE_SOURCE_DIR}/modules"
    "${CMAKE_BINARY_DIR}/modules"
	"${CMAKE_BINARY_DIR}/modules/dom1"
    "${CMAKE_SOURCE_DIR}/tests/dom1"
)

CORAL_GENERATE_MODULE( _GENERATED_FILES dom )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}/generated )

FILE( GLOB _HEADERS *.h )
FILE( GLOB _SOURCES Company.cpp Developer.cpp Project.cpp Manager.cpp )

ADD_LIBRARY( dom1 MODULE EXCLUDE_FROM_ALL ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )

CORAL_MODULE_TARGET( "dom1.dom" dom1 )

set_property( TARGET dom1 PROPERTY LIBRARY_OUTPUT_NAME dom )

SET_TARGET_PROPERTIES( dom1 PROPERTIES
	PROJECT_LABEL "Module DOM version 1"
)

################################################################################
# Build the test executable
################################################################################

INCLUDE_DIRECTORIES( ${GTEST_INCLUDE_DIRS}
					${DOM1_BINARY_DIR}/generated
					${DOM1_SOURCE_DIR}
					 ${CA_SOURCE_DIR}
                     ${CA_BINARY_DIR}/generated
					)

# Pass the CORAL_PATH as a precompiler definition
CORAL_GET_PATH_STRING( CORAL_PATH_STR )
SET_PROPERTY( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS "CORAL_PATH=\"${CORAL_PATH_STR}\"" )

# Gather test source files in the current directory
FILE( GLOB _TESTS *Tests.cpp )
FILE( GLOB _AUX_SOURCE CompanySpace.h CompanySpace.cpp )

# Create test executable
ADD_EXECUTABLE( tests_dom1 EXCLUDE_FROM_ALL Main.cpp ${_TESTS} ${_AUX_SOURCE} )
ADD_DEPENDENCIES( tests_dom1 dom1 ca )

CORAL_TARGET( tests_dom1 )

SET_TARGET_PROPERTIES( tests_dom1 PROPERTIES
	PROJECT_LABEL "Tests - DOM version 1"
)

TARGET_LINK_LIBRARIES( tests_dom1 ${GTEST_LIBRARIES} )

source_group( "@Generated" FILES ${_GENERATED_FILES} )

################################################################################
# Register the test
################################################################################
FILE( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/output )
ADD_TEST(
	NAME tests_dom1
	COMMAND $<TARGET_FILE:tests_dom1> --gtest_output=xml:../output/TestsDom1$<CONFIGURATION>.xml
)

################################################################################
# If Valgrind is available, repeat the test checking for memory leaks
################################################################################
IF( VALGRIND_COMMAND )
	ADD_TEST(
		NAME tests_dom1_MemoryCheck
		COMMAND ${VALGRIND_COMMAND} --leak-check=full --show-reachable=yes --num-callers=15 --dsymutil=yes
		--log-file=${CMAKE_BINARY_DIR}/ValgrindDom1$<CONFIGURATION>.log --error-exitcode=13
		--suppressions=${CMAKE_SOURCE_DIR}/tests/valgrind.supp $<TARGET_FILE:tests_dom1>
	)
ENDIF()