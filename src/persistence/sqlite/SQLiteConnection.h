/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */
#ifndef _SQLITECONNECTION_H_
#define _SQLITECONNECTION_H_

#include <co/TypeTraits.h>
#include "SQLiteResultSet.h"
#include <co/Log.h>
#include <string>

// Forward Declarations:
extern "C"
{
	typedef struct sqlite3 sqlite3;
	typedef struct sqlite3_stmt sqlite3_stmt;
}

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

	void setStatement( sqlite3_stmt* stmt );
	
	void bind( int index, double value );

	inline void bind( int index, const std::string& value )
	{
		bind( index, value.c_str() );
	}

	void bind( int index, const char* value );

	void bind( int index, co::int16 value )
	{
		bind( index, static_cast<co::int32>(value) );
	}

	inline void bind( int index, co::uint16 value )
	{
		bind( index, static_cast<co::int32>(value) );
	}

	void bind( int index, co::int32 value );

	inline void bind( int index, co::uint32 value )
	{
		bind( index, static_cast<co::int64>(value) );
	}

	void bind( int index, co::int64 value );

	void execute( SQLiteResultSet& rs );

	void execute();

	void reset();

	void finalize();

private:
	sqlite3_stmt* _preparedStatement;

	void handleErrorCode( int errorCode );
	

};

class SQLiteConnection
{
public:
	SQLiteConnection();

	~SQLiteConnection();

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

};

} // namespace ca

#endif // _CA_IDBCONNECTION_H_
