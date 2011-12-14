#include "../ERMSpace.h"
#include "persistence/sqlite/SQLite.h"
#include <gtest/gtest.h>
#include <co/reserved/OS.h>
#include <cstdio>

class SQLiteConnectionTests : public ERMSpace {};

TEST_F( SQLiteConnectionTests, testOpenNonExistingDB )
{
	std::string fileName = "connTest.db";

	ca::SQLiteConnection sqliteDBConn;

	remove(fileName.c_str());

	EXPECT_NO_THROW( sqliteDBConn.open( fileName ) );

	EXPECT_TRUE( co::OS::isFile(fileName) );

}

TEST_F( SQLiteConnectionTests, testExecuteWithoutOpen )
{
	std::string fileName = "connTest.db";

	ca::SQLiteConnection sqliteDBConn;

	EXPECT_THROW( sqliteDBConn.prepare("CREATE TABLE A (fieldX INTEGER)").execute(), ca::SQLiteException );
	EXPECT_THROW( ca::SQLiteResult rs = sqliteDBConn.prepare("SELECT * FROM A").query(), ca::SQLiteException );
}

TEST_F( SQLiteConnectionTests, testCreateDatabase )
{
	std::string fileName = "connTest.db";

	remove( fileName.c_str() );

	ca::SQLiteConnection sqliteDBConn;

	EXPECT_NO_THROW( sqliteDBConn.open( fileName ) );
	
	EXPECT_TRUE( co::OS::isFile(fileName) );
	
	//attempt to create again the database
	EXPECT_THROW( sqliteDBConn.open( fileName ), ca::SQLiteException );
	
	sqliteDBConn.close();
}

TEST_F( SQLiteConnectionTests, testSuccessfullExecutes )
{
	std::string fileName = "connTest.db";

	remove( fileName.c_str() );

	ca::SQLiteConnection sqliteDBConn;

	EXPECT_NO_THROW( sqliteDBConn.open( fileName ) );

	EXPECT_NO_THROW( sqliteDBConn.prepare( "CREATE TABLE A (fieldX INTEGER, fieldY TEXT)" ).execute() );

	EXPECT_NO_THROW( sqliteDBConn.prepare( "INSERT INTO A VALUES (1, 'text1')" ).execute() );
	EXPECT_NO_THROW( sqliteDBConn.prepare( "INSERT INTO A VALUES (2, 'text2')" ).execute() );
	EXPECT_NO_THROW( sqliteDBConn.prepare( "INSERT INTO A VALUES (3, 'text3')" ).execute() );
	
	EXPECT_NO_THROW( ca::SQLiteResult rs = sqliteDBConn.prepare( "SELECT * FROM A" ).query() );
	
	sqliteDBConn.close();
}

TEST_F( SQLiteConnectionTests, openExistingDB )
{
	std::string fileName = "connTest.db";

	ca::SQLiteConnection sqliteDBConn;

	remove( fileName.c_str() );

	sqliteDBConn.open( fileName );

	EXPECT_NO_THROW( sqliteDBConn.close() );

	EXPECT_NO_THROW( sqliteDBConn.open( fileName ) );
	
	//you cannot open an already opened database
	EXPECT_THROW( sqliteDBConn.open( fileName ), ca::SQLiteException );

	EXPECT_NO_THROW( sqliteDBConn.close() );

	//test harmless double close
	EXPECT_NO_THROW( sqliteDBConn.close() );

}

TEST_F( SQLiteConnectionTests, closeDBFail )
{
	std::string fileName = "connTest.db";

	ca::SQLiteConnection sqliteDBConn;

	remove(fileName.c_str());

	sqliteDBConn.open( fileName );

	EXPECT_NO_THROW(sqliteDBConn.prepare("CREATE TABLE A (fieldX INTEGER, fieldY TEXT)").execute());
	EXPECT_NO_THROW(sqliteDBConn.prepare("INSERT INTO A VALUES (1, 'text1')").execute());

	ca::SQLiteStatement stmt = sqliteDBConn.prepare( "SELECT * FROM A" );

	//not finalized IResultSet
	EXPECT_THROW(sqliteDBConn.close(), ca::SQLiteException);

	
}

TEST_F( SQLiteConnectionTests, preparedStatementTest )
{
	std::string fileName = "testStatement.db";

	ca::SQLiteConnection sqliteDBConn;

	remove(fileName.c_str());

	sqliteDBConn.open( fileName );

	EXPECT_NO_THROW(sqliteDBConn.prepare("CREATE TABLE A (fieldX INTEGER, fieldY TEXT)").execute());
	ca::SQLiteStatement stmt = sqliteDBConn.prepare("INSERT INTO A VALUES (?, ?)" );

	std::string str("value");

	EXPECT_NO_THROW( stmt.bind( 1, 1 ));
	EXPECT_NO_THROW( stmt.bind( 2, "value" ));
	EXPECT_NO_THROW( stmt.execute() );

	ca::SQLiteStatement stmt2 = sqliteDBConn.prepare("SELECT * FROM A WHERE fieldX =  ?" );
	EXPECT_NO_THROW( stmt2.bind( 1, 1 ) );
	ca::SQLiteResult rs = stmt2.query();

	EXPECT_TRUE( rs.next() );

	EXPECT_EQ( rs.getString( 0 ), "1" );
	EXPECT_EQ( rs.getString( 1 ), "value" );

	// unfinalized IResultSet
	EXPECT_THROW( sqliteDBConn.close(), ca::SQLiteException );

	stmt.finalize();
	stmt2.finalize();

	EXPECT_NO_THROW( sqliteDBConn.close() );
}
