################################################################################
# More tests for the Coral core
################################################################################

project( TESTS_DOM1 )

################################################################################
# Build the 'DOM1' module
################################################################################

set( CORAL_PATH ${CORAL_PATH}
    "${CMAKE_SOURCE_DIR}/tests/dom1"
	"${CMAKE_BINARY_DIR}/modules/dom1"
)

CORAL_GENERATE_MODULE( _GENERATED_FILES dom )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/generated )

file( GLOB _HEADERS *.h )
file( GLOB _SOURCES Company.cpp Developer.cpp Project.cpp Manager.cpp )

add_library( dom1 MODULE EXCLUDE_FROM_ALL ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )

set( CORAL_MODULE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/modules/dom1" )
CORAL_MODULE_TARGET( "dom" dom1 )
unset( CORAL_MODULE_OUTPUT_DIR )

set_target_properties( dom1 PROPERTIES
	PROJECT_LABEL "Module DOM version 1"
)

################################################################################
# Build the test executable
################################################################################

include_directories(
	${GTEST_INCLUDE_DIRS}
	${DOM1_SOURCE_DIR}
	${DOM1_BINARY_DIR}/generated
	${CA_SOURCE_DIR}
	${CA_BINARY_DIR}/generated
)

# Pass the CORAL_PATH as a precompiler definition
CORAL_GET_PATH_STRING( CORAL_PATH_STR )
set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS "CORAL_PATH=\"${CORAL_PATH_STR}\"" )

# Gather test source files in the current directory
file( GLOB _TESTS *Tests.cpp )
file( GLOB _AUX_SOURCE CompanySpace.h CompanySpace.cpp )

# Create test executable
add_executable( tests_dom1 EXCLUDE_FROM_ALL Main.cpp ${_TESTS} ${_AUX_SOURCE} )
add_dependencies( tests_dom1 dom1 ca )

CORAL_TARGET( tests_dom1 )

set_target_properties( tests_dom1 PROPERTIES
	PROJECT_LABEL "Tests - DOM version 1"
)

target_link_libraries( tests_dom1 ${GTEST_LIBRARIES} )

source_group( "@Generated" FILES ${_GENERATED_FILES} )

################################################################################
# If Valgrind is available, repeat the test checking for memory leaks
################################################################################

if( VALGRIND_COMMAND )
	add_test(
		NAME tests_dom1_MemoryCheck
		COMMAND ${CMAKE_COMMAND} chdir -E ${CMAKE_BINARY_DIR}/bdEvoTest
		${VALGRIND_COMMAND} --leak-check=full --show-reachable=yes --num-callers=15 --dsymutil=yes
		--log-file=${CMAKE_BINARY_DIR}/ValgrindDom1$<CONFIGURATION>.log --error-exitcode=13
		--suppressions=${CMAKE_SOURCE_DIR}/tests/valgrind.supp $<TARGET_FILE:tests_dom1>
	)
	CORAL_TEST_ENVIRONMENT( tests_dom1_MemoryCheck )
endif()
