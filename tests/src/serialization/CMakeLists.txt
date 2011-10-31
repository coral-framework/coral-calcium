################################################################################
# serialization
################################################################################

project( SERIALIZATION )

# Run the coral compiler on the module
CORAL_GENERATE_MODULE( _GENERATED_FILES serialization )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}/generated ${CMAKE_CURRENT_SOURCE_DIR} )

FILE( GLOB _HEADERS *.h )
FILE( GLOB _SOURCES *.cpp )

ADD_LIBRARY( serialization MODULE ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )
ADD_DEPENDENCIES( serialization ca )

CORAL_TARGET( serialization )
CORAL_MODULE_TARGET( "serialization" serialization )

SET_TARGET_PROPERTIES( erm PROPERTIES
	PROJECT_LABEL "Module Serialization"
)

TARGET_LINK_LIBRARIES( serialization ${CORAL_LIBRARIES} )

################################################################################
# Source Groups
################################################################################

SOURCE_GROUP( "@Generated" FILES ${_GENERATED_FILES} ${_HEADERS})
