################################################################################
# Calcium Module ("ca")
################################################################################

project( CA )

################################################################################
# Build the Module
################################################################################

CORAL_GENERATE_MODULE( _GENERATED_FILES ca lua.IState )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/generated )

file( GLOB coreFiles *.h *.cpp )
file( GLOB persistenceFiles persistence/*.h persistence/*.cpp )
file( GLOB sqliteFiles persistence/sqlite/* )

add_library( ca MODULE ${coreFiles} ${persistenceFiles} ${sqliteFiles} ${_GENERATED_FILES} )

CORAL_MODULE_TARGET( "ca" ca )

################################################################################
# Source Groups
################################################################################

source_group( "@Generated" FILES ${_GENERATED_FILES} )

source_group( "Core" FILES ${coreFiles} )
source_group( "Persistence" FILES ${persistenceFiles} )
source_group( "Persistence\\SQLite" FILES ${sqliteFiles} )

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

install( FILES "${CMAKE_BINARY_DIR}/modules/ca/ca.pdb"
	DESTINATION "modules/ca"
	CONFIGURATIONS RelWithDebInfo
	COMPONENT "Calcium Module"
	OPTIONAL
)

# install module files
install( DIRECTORY ${CMAKE_SOURCE_DIR}/modules/ca
	DESTINATION "modules"
	CONFIGURATIONS Release RelWithDebInfo
	COMPONENT "Calcium Module"
)

