#include "SQLiteDBConnection.h"
#include "SQLiteResultSet.h"
#include "IDBConnection.h"
#include "IResultSet.h"

#include "sqlite3.h"
#include <ca/DBException.h>


namespace ca
{
	SQLiteDBConnection::SQLiteDBConnection()
	{
		_db = 0;
	}

	SQLiteDBConnection::~SQLiteDBConnection()
	{
		if( _db ) 
		{
			close();
		}
	}

	void SQLiteDBConnection::createDatabase()
	{
		if( fileExists( _fileName.c_str() ) )
		{
			throw ca::DBException("Create database failed. File already exists");
		}

		if( !sqlite3_open(_fileName.c_str(), &_db) == SQLITE_OK )
		{
			throw ca::DBException("Create database failed");
		}
	}

	void SQLiteDBConnection::execute( const std::string& insertOrUpdateSQL ) 
	{
		if(!_db)
		{
			throw ca::DBException("Database not connected. Cannot execute command");
		}
		char* error;
		sqlite3_exec(_db, insertOrUpdateSQL.c_str(), 0, 0, &error);

		if(error)
		{
			throw ca::DBException(error);
		}
	}

	ca::IResultSet* SQLiteDBConnection::executeQuery( const std::string& querySQL )
	{ 
		if(!_db)
		{
			throw ca::DBException( "Database not connected. Cannot execute command" );
		}

		int resultCode = sqlite3_prepare_v2( _db, querySQL.c_str(), -1, &_statement, 0 );

		if(resultCode != SQLITE_OK)
		{
			char errorMsg[100];
			sprintf( errorMsg, "query failed: %s.", sqlite3_errmsg(_db) );
			throw ca::DBException(errorMsg);
		}

		SQLiteResultSet* resultSetSQLite = new SQLiteResultSet();
						
		resultSetSQLite->setStatement(_statement);
			
		return resultSetSQLite;
	}

	void SQLiteDBConnection::open()
	{
		if(!fileExists(_fileName.c_str()))
		{
			throw ca::DBException( "Open database failed. Attempt to open non existing file" );
		}

		if( _db )
		{
			throw ca::DBException( "Open database failed. Database already opened" );
		}
			
		if(!sqlite3_open(_fileName.c_str(), &_db) == SQLITE_OK)
		{
			throw ca::DBException( "Open database failed" );
		}
	}

	void SQLiteDBConnection::close()
	{
		int closeCode = sqlite3_close(_db);
		if(closeCode == SQLITE_OK)
		{
			_db = 0;
		}
		else 
		{
			throw ca::DBException("Could not close database. Check for not finalized IResultSets");
		}
	}

	const std::string& SQLiteDBConnection::getFileName()
	{
		return _fileName;
	}

	void SQLiteDBConnection::setFileName( const std::string& name )
	{
		_fileName = name;
	}

	bool SQLiteDBConnection::isConnected()
	{
		return _db != NULL;
	}

	
	bool SQLiteDBConnection::fileExists(const char * filePath)
	{
		FILE* file = fopen(filePath, "r");
		bool result = file != 0;
		if(result)
		{
			fclose(file);
		}
		return result;
	}
};