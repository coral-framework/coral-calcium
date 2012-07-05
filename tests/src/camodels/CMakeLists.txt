################################################################################
# camodels modules
################################################################################

project( CAMODELS )

# Run the coral compiler on the module
CORAL_GENERATE_MODULE( _GENERATED_FILES camodels )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/generated )

file( GLOB _HEADERS *.h )
file( GLOB _SOURCES *.cpp )

add_library( camodels MODULE EXCLUDE_FROM_ALL ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )
add_dependencies( camodels erm ca )

CORAL_MODULE_TARGET( "camodels" camodels )

set_target_properties( camodels PROPERTIES
	PROJECT_LABEL "camodels"
)

################################################################################
# Source Groups
################################################################################

source_group( "@Generated" FILES ${_GENERATED_FILES} )
