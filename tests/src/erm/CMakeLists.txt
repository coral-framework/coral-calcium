################################################################################
# ERM - Entity Relationship Model, an auxiliary test module
################################################################################

project( ERM )

# Run the coral compiler on the module
CORAL_GENERATE_MODULE( _GENERATED_FILES erm )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/generated )

file( GLOB _HEADERS *.h )
file( GLOB _SOURCES *.cpp )

add_library( erm MODULE EXCLUDE_FROM_ALL ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )
add_dependencies( erm ca )

CORAL_MODULE_TARGET( "erm" erm )

set_target_properties( erm PROPERTIES
	PROJECT_LABEL "Module ERM"
)

################################################################################
# Source Groups
################################################################################

source_group( "@Generated" FILES ${_GENERATED_FILES} )
