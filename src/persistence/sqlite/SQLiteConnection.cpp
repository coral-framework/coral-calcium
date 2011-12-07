/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SQLiteConnection.h"
#include "SQLiteResultSet.h"
#include "SQLiteException.h"
#include "sqlite3.h"
#include <sstream>

namespace ca {
	
SQLiteConnection::SQLiteConnection()
{
	_db = 0;
}

SQLiteConnection::~SQLiteConnection()
{
	if( _db ) 
	{
		close();
	}
}

void SQLiteConnection::checkConnection()
{
	if(!_db)
	{
		throw ca::SQLiteException("Database not connected. Cannot execute command");
	}
}

void SQLiteConnection::createDatabase()
{
	if( fileExists( _fileName.c_str() ) )
	{
		throw ca::SQLiteException("Create database failed. File already exists");
	}

	if( !sqlite3_open(_fileName.c_str(), &_db) == SQLITE_OK )
	{
		throw ca::SQLiteException("Create database failed");
	}
}

void SQLiteConnection::execute( const std::string& insertOrUpdateSQL ) 
{
	checkConnection();

	char* error;
	sqlite3_exec(_db, insertOrUpdateSQL.c_str(), 0, 0, &error);

	if(error)
	{
		throw ca::SQLiteException(error);
	}
}

void SQLiteConnection::executeQuery( const std::string& querySQL, ca::SQLiteResultSet& resultSetSQLite )
{ 
	checkConnection();

	int resultCode = sqlite3_prepare_v2( _db, querySQL.c_str(), -1, &_statement, 0 );

	if(resultCode != SQLITE_OK)
	{
		CORAL_THROW( ca::SQLiteException, "Query Failed: " << sqlite3_errmsg( _db ) );
	}

	resultSetSQLite.setStatement( _statement, false );
}

void SQLiteConnection::createPreparedStatement( const std::string& querySQL, ca::SQLitePreparedStatement& stmt )
{
	if(!_db)
	{
		throw ca::SQLiteException( "Database not connected. Cannot execute command" );
	}

	int resultCode = sqlite3_prepare_v2( _db, querySQL.c_str(), -1, &_statement, 0 );

	if(resultCode != SQLITE_OK)
	{
		CORAL_THROW( ca::SQLiteException, "Query Failed: " << sqlite3_errmsg( _db ) );
	}

	stmt.setStatement(_statement);
}

void SQLiteConnection::open()
{
	if( !fileExists( _fileName.c_str() ) )
	{
		throw ca::SQLiteException( "Open database failed. Attempt to open non existing file" );
	}

	if( _db )
	{
		throw ca::SQLiteException( "Open database failed. Database already opened" );
	}
			
	if(!sqlite3_open(_fileName.c_str(), &_db) == SQLITE_OK)
	{
		throw ca::SQLiteException( "Open database failed" );
	}
}

void SQLiteConnection::close()
{
	int closeCode = sqlite3_close(_db);
	if(closeCode == SQLITE_OK)
	{
		_db = 0;
	}
	else
	{
		throw ca::SQLiteException("Could not close database. Check for not finalized IResultSets");
	}
}

const std::string& SQLiteConnection::getFileName()
{
	return _fileName;
}

void SQLiteConnection::setFileName( const std::string& name )
{
	_fileName = name;
}

bool SQLiteConnection::isConnected()
{
	return _db != NULL;
}

	
bool SQLiteConnection::fileExists(const char * filePath)
{
	FILE* file = fopen(filePath, "r");
	bool result = file != 0;
	if(result)
	{
		fclose(file);
	}
	return result;
}

} // namespace ca
