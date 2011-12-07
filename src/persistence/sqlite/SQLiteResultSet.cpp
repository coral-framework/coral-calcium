/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SQLiteResultSet.h"
#include "sqlite3.h"
#include <co/Coral.h>
#include "SQLiteException.h"
#include <string>

namespace ca {
	
bool SQLiteResultSet::next()
{
	int status = sqlite3_step( _stmt );
	if( status == SQLITE_ERROR )
	{
		throw ca::SQLiteException("error on getting next result on ResultSet");
	}
	if(status == SQLITE_DONE)
	{
		return false;
	}
	return true;
}
	
const std::string SQLiteResultSet::getValue( co::uint32 columnIndex )
{
	if( columnIndex >= _columnCount )
	{
		throw ca::SQLiteException("invalid column index on getValue");
	}
	const unsigned char* value = sqlite3_column_text(_stmt, columnIndex);
		
	if( value == NULL )
	{
		throw ca::SQLiteException("ResultSet not pointing to a valid row. Check if next was called");
	}
	std::string str((char*)value);
	return str;
}

void SQLiteResultSet::setStatement( sqlite3_stmt* stmt, bool prepared )
{ 
	assert(stmt);
	_stmt = stmt;
	_isPreparedStatement = prepared;
	_columnCount = sqlite3_column_count(stmt);
}

void SQLiteResultSet::finalize()
{
	if( _stmt && !_isPreparedStatement )
	{
		int error = sqlite3_finalize(_stmt);

		if( error != SQLITE_OK )
		{
			throw ca::SQLiteException("Could not finalize ResultSet");
		}

	}
	_stmt = 0;
}

} // namespace ca
