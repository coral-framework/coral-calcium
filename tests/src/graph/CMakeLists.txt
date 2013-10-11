################################################################################
# Graph - an auxiliary test module
################################################################################

project( GRAPH )

# Run the coral compiler on the module
CORAL_GENERATE_MODULE( _GENERATED_FILES graph )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/generated )

file( GLOB _HEADERS *.h )
file( GLOB _SOURCES *.cpp )

add_library( graph MODULE EXCLUDE_FROM_ALL ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )
add_dependencies( graph ca )

CORAL_MODULE_TARGET( "graph" graph )

set_target_properties( graph PROPERTIES
	PROJECT_LABEL "Module Graph"
)

################################################################################
# Source Groups
################################################################################

source_group( "@Generated" FILES ${_GENERATED_FILES} )
