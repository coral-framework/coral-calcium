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
		throw ca::IOException( "error getting next result in SQLiteResult" );

	return status != SQLITE_DONE;
}

void SQLiteResult::fetchRow()
{
	if( !next() )
		throw ca::IOException( "unexpected empty SQLiteResult" );
}

bool SQLiteResult::hasData( int column )
{
	int count = sqlite3_data_count( _stmt );
	return count > 0 && column < count;
}

const char* SQLiteResult::getString( int column )
{
	assert( hasData( column ) );
	return reinterpret_cast<const char*>( sqlite3_column_text( _stmt, column ) );
}

co::uint32 SQLiteResult::getUint32( int column )
{
	assert( hasData( column ) );
	int v = sqlite3_column_int( _stmt, column );
	assert( v >= 0 );
	return static_cast<co::uint32>( v );
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

void SQLiteStatement::bind( int index, co::int32 value )
{
	handleErrorCode( sqlite3_bind_int( _stmt, index, value ) );
}

void SQLiteStatement::bind( int index, double value )
{
	handleErrorCode( sqlite3_bind_double( _stmt, index, value ));
}

void SQLiteStatement::bind( int index, const char* value )
{
	handleErrorCode( sqlite3_bind_text( _stmt, index, value, -1, NULL ));
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
		sqlite3_reset( _stmt );
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

void SQLiteConnection::open( const std::string& fileName )
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
		throw ca::IOException( "Could not close database. Check for unfinalized SQLiteResults" );

	_db = 0;
}

SQLiteStatement SQLiteConnection::prepare( const char* sql )
{
	if( !_db )
		throw ca::IOException( "Database not connected. Cannot execute command" );

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
		throw ca::IOException( "Database not connected. Cannot execute command" );
}

} // namespace ca
