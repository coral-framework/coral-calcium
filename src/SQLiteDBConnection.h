/*******************************************************************************
** C++ mapping generated for type 'ca.IDBConnection'
**
** Created: Fri Nov 04 15:48:29 2011
**      by: Coral Compiler version 0.7.0
**
** WARNING! All changes made in this file will be lost when recompiling!
********************************************************************************/

#ifndef _CA_SQLITEDBCONNECTION_H_
#define _CA_SQLITEDBCONNECTION_H_

#include <co/TypeTraits.h>
#include "IDBConnection.h"
#include "sqlite3.h"

#include <string>
using namespace std;

// Forward Declarations:
namespace ca {
	class IResultSet;
} // namespace ca
// End Of Forward Declarations

// ca.IDBConnection Mapping:
namespace ca {

class SQLiteDBConnection : public IDBConnection
{
public:

	SQLiteDBConnection();

	virtual ~SQLiteDBConnection();

	void close();

	void createDatabase();

	void execute( const std::string& insertOrUpdateSQL );

	ca::IResultSet* executeQuery( const std::string& querySQL );

	bool isConnected();

	void open();

	void setFileName(const std::string& fileName);

	const std::string& getFileName();

private:
	sqlite3* _db;
	std::string _fileName;
	sqlite3_stmt *_statement;

	bool fileExists(const char * filePath);
};

} // namespace ca

#endif // _CA_IDBCONNECTION_H_
