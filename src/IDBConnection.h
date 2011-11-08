/*******************************************************************************
** C++ mapping generated for type 'ca.IDBConnection'
**
** Created: Fri Nov 04 15:48:29 2011
**      by: Coral Compiler version 0.7.0
**
** WARNING! All changes made in this file will be lost when recompiling!
********************************************************************************/

#ifndef _CA_IDBCONNECTION_H_
#define _CA_IDBCONNECTION_H_

#include <co/TypeTraits.h>

// Forward Declarations:
namespace ca {
	class IResultSet;
} // namespace ca
// End Of Forward Declarations

// ca.IDBConnection Mapping:
namespace ca {

class IDBConnection
{
public:
	virtual ~IDBConnection() {;}

	virtual void close() = 0;

	virtual void createDatabase() = 0;

	virtual void execute( const std::string& insertOrUpdateSQL ) = 0;

	virtual ca::IResultSet* executeQuery( const std::string& querySQL ) = 0;

	virtual bool isConnected() = 0;

	virtual void open() = 0;
};

} // namespace ca

#endif // _CA_IDBCONNECTION_H_
