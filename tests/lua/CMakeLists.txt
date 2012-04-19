################################################################################
# Run the test scripts with CTest
################################################################################

set( CORAL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CORAL_PATH} )

CORAL_ADD_TEST( tests_lua testkit.Run ${CMAKE_CURRENT_SOURCE_DIR} -o "output/tests_lua_$<CONFIGURATION>.xml" )
