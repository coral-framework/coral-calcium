################################################################################
# Common settings for all test subprojects
################################################################################

# Add the tests 'module' dir to the CORAL_PATH
SET( CORAL_PATH ${CORAL_PATH} "${CMAKE_SOURCE_DIR}/tests/modules" )

# Pass the CORAL_PATH as a precompiler definition to all subprojects
CORAL_GET_PATH_STRING( CORAL_PATH_STR )
SET_PROPERTY( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS "CORAL_PATH=\"${CORAL_PATH_STR}\"" )

# Auxiliary Modules
ADD_SUBDIRECTORY( erm )

# The test executable
ADD_SUBDIRECTORY( tests )
