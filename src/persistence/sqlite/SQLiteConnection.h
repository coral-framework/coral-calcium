/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _SQLITECONNECTION_H_
#define _SQLITECONNECTION_H_

#include <co/TypeTraits.h>
#include "sqlite3.h"
#include "SQLiteResultSet.h"

#include <co/Log.h>

#include <string>

// Forward Declarations:
namespace ca {
	class IResultSet;
} // namespace ca
// End Of Forward Declarations

// ca.IDBConnection Mapping:
namespace ca {

class SQLitePreparedStatement 
{
public:
	SQLitePreparedStatement()
	{
		_preparedStatement = NULL;
	} 
	
	~SQLitePreparedStatement()
	{
		finalize();
	}

	void setStatement( sqlite3_stmt* stmt )
	{
		finalize();
		_preparedStatement = stmt;
	}

	void setDouble( co::uint32 index, double doubleValue )
	{
		handleErrorCode( sqlite3_bind_double( _preparedStatement, index, doubleValue ));
	}

	void setString( co::uint32 index, const char* strValue )
	{
		handleErrorCode( sqlite3_bind_text( _preparedStatement, index, strValue, -1, NULL ));
	}

	void setInt( co::uint32 index, int intValue )
	{
		handleErrorCode( sqlite3_bind_int( _preparedStatement, index, intValue ) );
	}

	void setInt64( co::uint32 index, co::int64 intValue )
	{
		handleErrorCode( sqlite3_bind_int64( _preparedStatement, index, intValue ) );
	}

	void execute( SQLiteResultSet& rs )
	{
		rs.setStatement( _preparedStatement, true );
	}

	void execute()
	{
		handleErrorCode( sqlite3_step( _preparedStatement ) );
	}

	void reset()
	{
		if( _preparedStatement )
		{
			sqlite3_reset( _preparedStatement );
		}
	}

	void finalize()
	{
		if( _preparedStatement )
		{
			sqlite3_finalize( _preparedStatement );
			_preparedStatement = NULL;
		}
	}

private:
	sqlite3_stmt* _preparedStatement;

	void handleErrorCode( int errorCode )
	{
		if( errorCode != SQLITE_OK && errorCode != SQLITE_DONE )
		{
			CORAL_DLOG(INFO) << errorCode;
		}
	}

};

class SQLiteConnection
{
public:
	SQLiteConnection();

	virtual ~SQLiteConnection();

	void close();

	void createDatabase();

	void execute( const std::string& insertOrUpdateSQL );

	void executeQuery( const std::string& querySQL, ca::SQLiteResultSet& rs );

	void createPreparedStatement( const std::string& querySQL, ca::SQLitePreparedStatement& stmt );

	bool isConnected();

	void open();

	void setFileName(const std::string& fileName);

	const std::string& getFileName();

private:
	sqlite3* _db;
	std::string _fileName;
	sqlite3_stmt* _statement;

	void checkConnection();

	bool fileExists(const char * filePath);
};

} // namespace ca

#endif // _CA_IDBCONNECTION_H_
