/*******************************************************************************
** C++ mapping generated for type 'ca.IDBConnection'
**
** Created: Fri Nov 04 15:48:29 2011
**      by: Coral Compiler version 0.7.0
**
** WARNING! All changes made in this file will be lost when recompiling!
********************************************************************************/

#ifndef _CA_SQLiteConnection_H_
#define _CA_SQLiteConnection_H_

#include <co/TypeTraits.h>
#include "sqlite3.h"
#include "SQLiteResultSet.h"

#include <string>

// Forward Declarations:
namespace ca {
	class IResultSet;
} // namespace ca
// End Of Forward Declarations

// ca.IDBConnection Mapping:
namespace ca {

class SQLiteConnection
{
public:

	SQLiteConnection();

	virtual ~SQLiteConnection();

	void close();

	void createDatabase();

	void execute( const std::string& insertOrUpdateSQL );

	void executeQuery( const std::string& querySQL, ca::SQLiteResultSet& rs );

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
