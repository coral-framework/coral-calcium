/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_DBEXCEPTION_H_
#define _CA_DBEXCEPTION_H_

#include <co/Exception.h>

namespace ca {

class DBException : public co::Exception
{
public:
	DBException()
	{;}

	DBException( const std::string& message )
		: co::Exception( message )
	{;}

	virtual ~DBException() throw()
	{;}
};

} // namespace ca

#endif // _CA_DBEXCEPTION_H_
