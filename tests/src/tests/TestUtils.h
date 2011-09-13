/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _TESTUTILS_H_
#define _TESTUTILS_H_

#include <gtest/gtest.h>

#define ASSERT_EXCEPTION( code, expectedErrorMsg ) \
	try { code; FAIL() << "an exception should have been raised"; } \
	catch( const std::exception& e ) { std::string raisedMsg( e.what() ); \
		if( raisedMsg.find( expectedErrorMsg ) == std::string::npos ) \
			FAIL()	<< "raised message (\"" << raisedMsg << "\") does not contain the expected message (\"" \
					<< expectedErrorMsg << "\")"; \
	}

#endif // _TESTUTILS_H_
