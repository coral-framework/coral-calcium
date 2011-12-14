/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SQLite.h"
#include "sqlite3.h"
#include <sstream>
#include <ca/IOException.h>

namespace ca {

/************************************************************************/
/* SQLiteResult                                                         */
/************************************************************************/

SQLiteResult::SQLiteResult( sqlite3_stmt* stmt )
{
	assert( stmt );
	_stmt = stmt;
}

SQLiteResult::SQLiteResult( const SQLiteResult& o )
{
	_stmt = o._stmt;
	o._stmt = NULL;
}

bool SQLiteResult::next()
{
	int status = sqlite3_step( _stmt );
	if( status == SQLITE_ERROR )
	{
		throw ca::IOException("error on getting next result on ResultSet");
	}
	return status != SQLITE_DONE;
}

bool SQLiteResult::hasData( int column )
{
	int count = sqlite3_data_count( _stmt );
	return count > 0 && column < count;
}

const std::string SQLiteResult::getString( int column )
{
	assert( hasData( column ) );
	const unsigned char* value = sqlite3_column_text( _stmt, column );
	if( value == NULL )
	{
		return std::string();
	}
	std::string str( reinterpret_cast<const char*>( value ) );
	return str;
}

co::uint32 SQLiteResult::getUint32( int column )
{
	assert( hasData( column ) );
	return static_cast<co::uint32>( sqlite3_column_int64( _stmt, column ) );
}

/************************************************************************/
/* SQLiteStatement                                                      */
/************************************************************************/

SQLiteStatement::SQLiteStatement( sqlite3_stmt* stmt )
{
	assert( stmt );
	_stmt = stmt;
}

SQLiteStatement::SQLiteStatement( const SQLiteStatement& o)
{
	_stmt = o._stmt;
	o._stmt = NULL;
}

SQLiteStatement::~SQLiteStatement()
{
	finalize();
}

void SQLiteStatement::bind( int index, double value )
{
	handleErrorCode( sqlite3_bind_double( _stmt, index, value ));
}

void SQLiteStatement::bind( int index, const char* value )
{
	handleErrorCode( sqlite3_bind_text( _stmt, index, value, -1, NULL ));
}

void SQLiteStatement::bind( int index, co::int32 value )
{
	handleErrorCode( sqlite3_bind_int( _stmt, index, value ) );
}

void SQLiteStatement::bind( int index, co::int64 value )
{
	handleErrorCode( sqlite3_bind_int64( _stmt, index, value ) );
}

SQLiteResult SQLiteStatement::query()
{
	return SQLiteResult( _stmt );
}

void SQLiteStatement::execute()
{
	handleErrorCode( sqlite3_step( _stmt ) );
}

void SQLiteStatement::reset()
{
	if( _stmt )
	{
		sqlite3_reset( _stmt );
	}
}

void SQLiteStatement::finalize()
{
	if( _stmt )
	{
		sqlite3_finalize( _stmt );
		_stmt = NULL;
	}
}

void SQLiteStatement::handleErrorCode( int errorCode )
{
	if( errorCode != SQLITE_OK && errorCode != SQLITE_DONE )
	{
		CORAL_THROW( ca::IOException, "SQLite statement error "
			<< sqlite3_errmsg( sqlite3_db_handle( _stmt ) ) );
	}
}

/************************************************************************/
/* SQLiteConnection                                                     */
/************************************************************************/

SQLiteConnection::SQLiteConnection()
{
	_db = 0;
}

SQLiteConnection::~SQLiteConnection()
{
	close();
}

void SQLiteConnection::open(const std::string& fileName)
{
	if( isConnected() )
		throw ca::IOException( "Open database failed. Database already opened" );

	if( sqlite3_open( fileName.c_str(), &_db ) != SQLITE_OK )
		throw ca::IOException( "Open database failed" );

	prepare( "PRAGMA foreign_keys = ON" ).execute();
}

void SQLiteConnection::close()
{
	if( !_db )
		return;

	if( sqlite3_close( _db ) != SQLITE_OK )
		throw ca::IOException( "Could not close database. Check for unfinalized IResultSets" );

	_db = 0;
}

SQLiteStatement SQLiteConnection::prepare( const char* sql )
{
	if( !_db )
	{
		throw ca::IOException( "Database not connected. Cannot execute command" );
	}

	sqlite3_stmt* stmt;

	int resultCode = sqlite3_prepare_v2( _db, sql, -1, &stmt, 0 );
	if( resultCode != SQLITE_OK )
	{
		CORAL_THROW( ca::IOException, "Query Failed: " << sqlite3_errmsg( _db ) );
	}

	return SQLiteStatement( stmt );
}

void SQLiteConnection::checkConnection()
{
	if( !isConnected() )
	{
		throw ca::IOException( "Database not connected. Cannot execute command" );
	}
}

} // namespace ca
