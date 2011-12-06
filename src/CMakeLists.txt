################################################################################
# Calcium Module ("ca")
################################################################################

project( CA )

################################################################################
# Build the Module
################################################################################

CORAL_GENERATE_MODULE( _GENERATED_FILES ca lua.IState )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/generated )

file( GLOB HEADER_FILES *.h )
file( GLOB SOURCE_FILES *.cpp )
file( GLOB SQLITE_FILES sqlite/* )

add_library( ca MODULE ${HEADER_FILES} ${SOURCE_FILES} ${SQLITE_FILES} ${_GENERATED_FILES} )

CORAL_MODULE_TARGET( "ca" ca )

################################################################################
# Source Groups
################################################################################

source_group( "@Generated" FILES ${_GENERATED_FILES} )

file( GLOB coreFiles Model* Object* Service* Space* Universe* Module* )
source_group( "Core" FILES ${coreFiles} )

file( GLOB persistenceFiles *Persister* )
source_group( "Persistence" FILES ${persistenceFiles} )
source_group( "Persistence\\SQLite" FILES ${SQLITE_FILES} )

source_group( "Undo" FILES "UndoManager.cpp" )

################################################################################
# Install Rules
################################################################################

# install shared library
install( TARGETS ca
	DESTINATION "modules/ca"
	COMPONENT "Calcium Module"
)

install( FILES "${CMAKE_BINARY_DIR}/modules/ca/ca_debug.pdb"
	DESTINATION "modules/ca"
	CONFIGURATIONS Debug
	COMPONENT "Calcium Module"
	OPTIONAL
)

# install module files
install( DIRECTORY ${CMAKE_SOURCE_DIR}/modules/ca
	DESTINATION "modules"
	CONFIGURATIONS Release RelWithDebInfo
	COMPONENT "Calcium Module"
)
