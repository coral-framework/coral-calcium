################################################################################
# Build the Module
################################################################################

CORAL_GENERATE_MODULE( _GENERATED_FILES ca lua.IState )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}/generated )

FILE( GLOB _SOURCES *.cpp )
FILE( GLOB _HEADERS *.h )

ADD_LIBRARY( ca MODULE ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )

CORAL_DEFAULT_TARGET_PROPERTIES( ca )
CORAL_MODULE_TARGET_PROPERTIES( ca )

TARGET_LINK_LIBRARIES( ca ${CORAL_LIBRARIES} )

################################################################################
# Source Groups
################################################################################

SOURCE_GROUP( "@Generated" FILES ${_GENERATED_FILES} )
