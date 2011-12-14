/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_SQLITE_H_
#define _CA_SQLITE_H_

#include <co/Log.h>
#include <co/Exception.h>

// Forward Declarations:
extern "C"
{
	typedef struct sqlite3 sqlite3;
	typedef struct sqlite3_stmt sqlite3_stmt;
}

namespace ca {

/*!
	Class to iterate through SQLite query results (a list of rows).
	No data is available until the first call to next().
 */
class SQLiteResult
{
public:
	SQLiteResult( sqlite3_stmt* stmt );
	SQLiteResult( const SQLiteResult& o );

	/*!
		Moves to the next row in the result set.
		\return true if the next row was fetched, false if there are no more rows.
	*/
	bool next();

	//! Retrieves the value at \a column as a string.
	const std::string getString( int column );

	//! Retrieves the value at \a column as an uint32.
	co::uint32 getUint32( int column );

private:
	// forbid assignments
	SQLiteResult& operator=( const SQLiteResult& o );

	inline bool hasData( int column );

private:
	mutable sqlite3_stmt* _stmt;
};

/*!
	\brief Abstraction for a SQLite Prepared Statement.
	Values are assigned to parameters using the bind() methods.
 */
class SQLiteStatement 
{
public:
	SQLiteStatement( sqlite3_stmt* stmt );
	SQLiteStatement( const SQLiteStatement& o);
	~SQLiteStatement();

	//! Bind a double.
	void bind( int index, double value );

	//! Bind a string.
	inline void bind( int index, const std::string& value )
	{
		bind( index, value.c_str() );
	}

	//! Bind a literal/C-string.
	void bind( int index, const char* value );

	//! Bind an int16 (promoting to int32).
	void bind( int index, co::int16 value )
	{
		bind( index, static_cast<co::int32>(value) );
	}

	//! Bind an uint16 (promoting to int32).
	inline void bind( int index, co::uint16 value )
	{
		bind( index, static_cast<co::int32>(value) );
	}

	//! Bind an int32.
	void bind( int index, co::int32 value );

	//! Bind an uint32 (promoting to int64).
	inline void bind( int index, co::uint32 value )
	{
		bind( index, static_cast<co::int64>(value) );
	}

	//! Bind an int64.
	void bind( int index, co::int64 value );

	/*!
		Executes a SELECT statement.
		This statement must be valid as long as the SQLiteResult is being consulted.
	 */
	SQLiteResult query();

	/*!
		Executes a non-SELECT statement.
	 */
	void execute();

	//! Resets the statement so it can be reused. Parameters must be bound again.
	void reset();

	/*!
		Releases the statement. Called automatically when the object dies.
		Unfinalized statements prevent their SQLiteConnection from being closed.
	 */
	void finalize();

private:
	// forbid assignments
	SQLiteStatement& operator=( const SQLiteStatement& o );

	void handleErrorCode( int errorCode );

private:
	mutable sqlite3_stmt* _stmt;
};

/*!
	Connection to a SQLite database file.
 */
class SQLiteConnection
{
public:
	SQLiteConnection();
	~SQLiteConnection();

	inline bool isConnected() { return _db != NULL; }

	/*!
		Opens a connection to the database with the given \a fileName.
		If the database does not exist yet, it is created.
	 */
	void open( const std::string& fileName );

	ca::SQLiteStatement prepare( const char* sql );

	void close();

private:
	void checkConnection();

private:
	sqlite3* _db;
};

} // namespace ca

#endif // _CA_SQLITE_H_
