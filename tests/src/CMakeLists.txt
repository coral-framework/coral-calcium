################################################################################
# Common settings for all test subprojects
################################################################################

# Pass the CORAL_PATH as a precompiler definition to all subprojects
CORAL_GET_PATH_STRING( CORAL_PATH_STR )
set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
	"CORAL_PATH=\"${CORAL_ROOT}/modules,${CORAL_PATH_STR}\"" )

# Auxiliary Modules
add_subdirectory( camodels )
add_subdirectory( erm )
add_subdirectory( serialization )

# The test executable
add_subdirectory( tests )
