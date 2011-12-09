/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */
#ifndef _CA_SQLITE_H_
#define _CA_SQLITE_H_

#include <co/TypeTraits.h>
#include <co/Log.h>
#include <co/Exception.h>
#include <string>

// Forward Declarations:
extern "C"
{
	typedef struct sqlite3 sqlite3;
	typedef struct sqlite3_stmt sqlite3_stmt;
}

namespace ca {

	/*!
		Exception class for SQLite errors
	*/

class SQLiteException : public co::Exception
{
public:
	SQLiteException()
	{;}

	SQLiteException( const std::string& message )
		: co::Exception( message )
	{;}

	virtual ~SQLiteException() throw()
	{;}
};


/*!
	Class to iterate through select sql clause results.
*/
class SQLiteResult
{
public:
	
	SQLiteResult( sqlite3_stmt* stmt );

	// copy constructor
	SQLiteResult( const SQLiteResult& o );
	
	~SQLiteResult(){} // empty

	/*!
		Gets the next row of query result.
		\returns true if it successfully fetched the next row, false if there isn't any rows to be fetched.
	*/

	bool next();
		
	const std::string getString( co::uint32 columnIndex );

	const co::uint32 getUint32( co::uint32 columnIndex );

private:
	SQLiteResult& operator=( const SQLiteResult& o );

private:
	mutable sqlite3_stmt* _stmt;
};

/*!
	Abstraction for sql statements.	
*/

class SQLiteStatement 
{
public:
	SQLiteStatement( sqlite3_stmt* stmt );
	
	
	SQLiteStatement( const SQLiteStatement& o);
	

	~SQLiteStatement()
	{
		finalize();
	}
	/*!
		bind parameters to the statement, according to the given parameter type
	*/
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

	/*!
		Executes a select statement. This statement must be valid as long as the SQLiteResult is being consulted.
	*/
	SQLiteResult query();

	/*!
		Executes a non select statement.
	*/
	void execute();

	/*!
		Resets the statement to its initial state. The parameters must be bound again, and the statement can be reused.
	*/
	void reset();

	/*!
	Releases the statement. If the statement is not finalized, it may prevent the closure of the associated SQLiteConnection.
	*/
	void finalize();

private:
	void handleErrorCode( int errorCode );
	SQLiteStatement& operator=( const SQLiteStatement& o );

private:
	mutable sqlite3_stmt* _stmt;
};

/*!

	Connection to a sqlite3 database file

*/

class SQLiteConnection
{
public:
	SQLiteConnection();

	~SQLiteConnection();

	/*!
		Open connection to the given file. 
		\param fileName database file to be connected to. if the file already exists, a connection is opened. Otherwise, a file is created.
	*/
	void open( const std::string& fileName );

	ca::SQLiteStatement prepare( const std::string& querySQL );

	bool isConnected();

	void close();

private:
	void checkConnection();

private:
	sqlite3* _db;
};

} // namespace ca

#endif // _CA_SQLITE_H_
