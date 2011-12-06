################################################################################
# serialization
################################################################################

project( SERIALIZATION )

# Run the coral compiler on the module
CORAL_GENERATE_MODULE( _GENERATED_FILES serialization )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/generated ${CMAKE_CURRENT_SOURCE_DIR} )

file( GLOB _HEADERS *.h )
file( GLOB _SOURCES *.cpp )

add_library( serialization MODULE ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )
add_dependencies( serialization ca )

CORAL_MODULE_TARGET( "serialization" serialization )

set_target_properties( erm PROPERTIES
	PROJECT_LABEL "Module Serialization"
)

target_link_libraries( serialization ${CORAL_LIBRARIES} )

################################################################################
# Source Groups
################################################################################

source_group( "@Generated" FILES ${_GENERATED_FILES} )
