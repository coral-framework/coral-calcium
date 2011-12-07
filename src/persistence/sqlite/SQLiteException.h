/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_SQLITEEXCEPTION_H_
#define _CA_SQLITEEXCEPTION_H_

#include <co/Exception.h>

namespace ca {

class SQLiteException : public co::Exception
{
public:
	SQLiteException()
	{;}

	SQLiteException( const std::string& message )
		: co::Exception( message )
	{;}

	virtual ~SQLiteException() throw()
	{;}
};

} // namespace ca

#endif // _CA_SQLITEEXCEPTION_H_
