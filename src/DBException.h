/*******************************************************************************
** C++ mapping generated for type 'ca.DBException'
**
** Created: Mon Nov 07 13:31:15 2011
**      by: Coral Compiler version 0.7.0
**
** WARNING! All changes made in this file will be lost when recompiling!
********************************************************************************/

#ifndef _CA_DBEXCEPTION_H_
#define _CA_DBEXCEPTION_H_

#include <exception>

using namespace std;

// ca.DBException Mapping:
namespace ca {

class DBException : public std::exception
{
public:
	DBException()
	{;}

	DBException( const std::string& message )
		: std::exception( message.c_str() )
	{;}

	virtual ~DBException() throw()
	{;}

};

} // namespace ca


#endif // _CA_DBEXCEPTION_H_
