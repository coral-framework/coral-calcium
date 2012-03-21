################################################################################
# More tests for the Coral core
################################################################################

project( TESTS_DOM2 )

################################################################################
# Build the 'DOM2' module
################################################################################

set( CORAL_PATH ${CORAL_PATH}
    "${CMAKE_SOURCE_DIR}/tests/dom2"
	"${CMAKE_BINARY_DIR}/modules/dom2"
)

CORAL_GENERATE_MODULE( _GENERATED_FILES dom )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/generated )

file( GLOB _HEADERS *.h )
file( GLOB _SOURCES Company.cpp Employee.cpp Product.cpp Service.cpp )

add_library( dom2 MODULE EXCLUDE_FROM_ALL ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )

set( CORAL_MODULE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/modules/dom2" )
CORAL_MODULE_TARGET( "dom" dom2 )
unset( CORAL_MODULE_OUTPUT_DIR )

set_target_properties( dom2 PROPERTIES
	PROJECT_LABEL "Module DOM version 2"
)

################################################################################
# Build the test executable
################################################################################

# Pass the CORAL_PATH as a precompiler definition
CORAL_GET_PATH_STRING( CORAL_PATH_STR )

set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS "CORAL_PATH=\"${CORAL_PATH_STR}\"" )

include_directories(
	${GTEST_INCLUDE_DIRS}
	${CA_SOURCE_DIR}
	${CA_BINARY_DIR}/generated
)

# Gather test source files in the current directory
file( GLOB _TESTS *Tests.cpp )

# Create test executable
add_executable( tests_dom2 EXCLUDE_FROM_ALL Main.cpp ${_TESTS} )
add_dependencies( tests_dom2 dom2 ca )

CORAL_TARGET( tests_dom2 )

set_target_properties( tests_dom2 PROPERTIES
	PROJECT_LABEL "Tests - DOM version 2"
)

target_link_libraries( tests_dom2 ${GTEST_LIBRARIES} )

source_group( "@Generated" FILES ${_GENERATED_FILES} )

################################################################################
# Register the test
################################################################################

################################################################################
# If Valgrind is available, repeat the test checking for memory leaks
################################################################################

if( VALGRIND_COMMAND )
	add_test(
		NAME tests_dom2_MemoryCheck
		COMMAND ${VALGRIND_COMMAND} --leak-check=full --show-reachable=yes --num-callers=15 --dsymutil=yes
		--log-file=${CMAKE_BINARY_DIR}/ValgrindDom2$<CONFIGURATION>.log --error-exitcode=13
		--suppressions=${CMAKE_SOURCE_DIR}/tests/valgrind.supp $<TARGET_FILE:tests_dom2>
	)
	CORAL_TEST_ENVIRONMENT( tests_dom2_MemoryCheck )
endif()
