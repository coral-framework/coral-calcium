################################################################################
# Calcium - Domain Model Framework
################################################################################

cmake_minimum_required( VERSION 2.6 )

project( CALCIUM )

################################################################################
# Setup Coral
################################################################################

# Resolve the CORAL_ROOT
file( TO_CMAKE_PATH "$ENV{CORAL_ROOT}" CORAL_ROOT )

# Add Coral's CMake dir to the CMAKE_MODULE_PATH
set( CMAKE_MODULE_PATH ${CORAL_ROOT}/cmake ${CMAKE_MODULE_PATH} )

# Load Coral's CMake package
find_package( Coral REQUIRED )

# Configure the coral path
set( CORAL_PATH
	"${CMAKE_BINARY_DIR}/modules"
	"${CMAKE_SOURCE_DIR}/modules"
)

include_directories( ${CORAL_INCLUDE_DIRS} )

################################################################################
# Installation
################################################################################

# enforce a safe default CMAKE_INSTALL_PREFIX
if( NOT CMAKE_INSTALL_PREFIX MATCHES "coral-calcium" )
	set( CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/coral-calcium" )
endif()

################################################################################
# Packaging
################################################################################

set( CPACK_PACKAGE_NAME					"coral-calcium" )
set( CPACK_PACKAGE_VERSION_MAJOR		"0" )
set( CPACK_PACKAGE_VERSION_MINOR		"1" )
set( CPACK_PACKAGE_VERSION_PATCH		"0" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY	"Calcium is a versatile and minimally intrusive domain object model framework." )

if( WIN32 )
	set( CPACK_GENERATOR "ZIP" )
else()
	set( CPACK_GENERATOR "TBZ2" )
endif()

include( CPack )

################################################################################
# Subdirectories
################################################################################

add_subdirectory( src )

enable_testing()
add_subdirectory( tests )
