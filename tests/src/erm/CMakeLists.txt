################################################################################
# ERM - Entity Relationship Model, an auxiliary test module
################################################################################

PROJECT( ERM )

# Run the coral compiler on the module
CORAL_GENERATE_MODULE( _GENERATED_FILES erm )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}/generated )

FILE( GLOB _HEADERS *.h )
FILE( GLOB _SOURCES *.cpp )

ADD_LIBRARY( erm MODULE EXCLUDE_FROM_ALL ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )
ADD_DEPENDENCIES( erm ca )

CORAL_MODULE_TARGET( "erm" erm )

SET_TARGET_PROPERTIES( erm PROPERTIES
	PROJECT_LABEL "Module ERM"
)

TARGET_LINK_LIBRARIES( erm ${CORAL_LIBRARIES} )

################################################################################
# Source Groups
################################################################################

SOURCE_GROUP( "@Generated" FILES ${_GENERATED_FILES} )
