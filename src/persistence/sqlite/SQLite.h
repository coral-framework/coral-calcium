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



class SQLiteResult
{
public:
	
	SQLiteResult( sqlite3_stmt* stmt );

	// copy constructor
	SQLiteResult( SQLiteResult& o );
	
	~SQLiteResult(){} // empty

	bool next();
		
	const std::string getString( co::uint32 columnIndex);

	const co::uint32 getUint32( co::uint32 columnIndex);

private:
	SQLiteResult& operator=( const SQLiteResult& o );

private:
	sqlite3_stmt* _stmt;
};



class SQLiteStatement 
{
public:
	SQLiteStatement( sqlite3_stmt* stmt );
	
	
	SQLiteStatement( SQLiteStatement& o);
	

	~SQLiteStatement()
	{
		finalize();
	}

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

	SQLiteResult query();

	void execute();

	void reset();

	void finalize();

private:
	void handleErrorCode( int errorCode );
	SQLiteStatement& operator=( const SQLiteStatement& o );

private:
	sqlite3_stmt* _stmt;
};

class SQLiteConnection
{
public:
	SQLiteConnection();

	~SQLiteConnection();

	void open( const std::string& fileName );

	ca::SQLiteStatement prepare( const std::string& querySQL );

	bool isConnected();

	void close();

private:
	void checkConnection();

private:
	sqlite3* _db;
	sqlite3_stmt* _statement;

};

} // namespace ca

#endif // _CA_SQLITE_H_
