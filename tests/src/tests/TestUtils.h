/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _TESTUTILS_H_
#define _TESTUTILS_H_

#include <gtest/gtest.h>

#define ASSERT_EXCEPTION( code, expectedMsg ) \
	try { code; FAIL() << "an exception should have been raised"; } \
	catch( const std::exception& e ) { std::string raisedMsg( e.what() ); \
		if( raisedMsg.find( expectedMsg ) == std::string::npos ) \
			FAIL()	<< "raised message (\"" << raisedMsg << "\") doesn't contain \"" << expectedMsg << "\""; \
	}

#define ASSERT_EXCEPTION2( code, msg1, msg2 ) \
	try { code; FAIL() << "an exception should have been raised"; } \
	catch( const std::exception& e ) { std::string raisedMsg( e.what() ); \
		if( raisedMsg.find( msg1 ) == std::string::npos ) \
			FAIL()	<< "raised message (\"" << raisedMsg << "\") doesn't contain \"" << msg1 << "\""; \
		else if( raisedMsg.find( msg2 ) == std::string::npos ) \
			FAIL()	<< "raised message (\"" << raisedMsg << "\") doesn't contain \"" << msg2 << "\""; \
	}

#endif // _TESTUTILS_H_
