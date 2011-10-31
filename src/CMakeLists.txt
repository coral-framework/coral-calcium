################################################################################
# Build the Module
################################################################################

CORAL_GENERATE_MODULE( _GENERATED_FILES ca lua.IState )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}/generated )

FILE( GLOB _SOURCES *.cpp )
FILE( GLOB _HEADERS *.h )

ADD_LIBRARY( ca MODULE ${_HEADERS} ${_SOURCES} ${_GENERATED_FILES} )

CORAL_MODULE_TARGET( "ca" ca )

TARGET_LINK_LIBRARIES( ca ${CORAL_LIBRARIES} )

################################################################################
# Source Groups
################################################################################

SOURCE_GROUP( "@Generated" FILES ${_GENERATED_FILES} )

################################################################################
# Install Rules
################################################################################

# install shared library
INSTALL( TARGETS ca
	DESTINATION "modules/ca"
	COMPONENT "Calcium Module"
)

INSTALL( FILES "${CMAKE_BINARY_DIR}/modules/ca/ca_debug.pdb"
	DESTINATION "modules/ca"
	CONFIGURATIONS Debug
	COMPONENT "Calcium Module"
	OPTIONAL
)

# install module files
INSTALL( DIRECTORY ${CMAKE_SOURCE_DIR}/modules/ca
	DESTINATION "modules"
	CONFIGURATIONS Release RelWithDebInfo
	COMPONENT "Calcium Module"
)
