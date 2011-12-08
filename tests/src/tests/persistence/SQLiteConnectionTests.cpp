#include "../ERMSpace.h"
#include "persistence/sqlite/SQLiteConnection.h"
#include "persistence/sqlite/SQLiteResultSet.h"
#include "persistence/sqlite/SQLiteException.h"
#include <ca/INamed.h>
#include <gtest/gtest.h>
#include <cstdio>

class SQLiteConnectionTests : public ERMSpace {};

TEST_F( SQLiteConnectionTests, testOpenNonExistingDB )
{
	std::string fileName = "SimpleSpaceSave.db";

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName(fileName);

	remove(fileName.c_str());

	EXPECT_THROW(sqliteDBConn.open(), ca::SQLiteException);
}

TEST_F( SQLiteConnectionTests, testExecuteWithoutOpen )
{
	std::string fileName = "SimpleSpaceSave.db";

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName(fileName);

	ca::SQLiteResultSet rs;
	
	EXPECT_THROW( sqliteDBConn.execute("CREATE TABLE A (fieldX INTEGER)"), ca::SQLiteException );
	EXPECT_THROW( sqliteDBConn.executeQuery("SELECT * FROM A", rs), ca::SQLiteException );
}

TEST_F( SQLiteConnectionTests, testCreateDatabase )
{
	std::string fileName = "SimpleSpaceSave.db";

	remove( fileName.c_str() );

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName(fileName);

	EXPECT_NO_THROW( sqliteDBConn.createDatabase() );
	FILE* fileDB = fopen( fileName.c_str(), "r" );
	EXPECT_TRUE( fileDB != 0 );
	fclose( fileDB );
	
	//attempt to create again the database
	EXPECT_THROW(sqliteDBConn.createDatabase(), ca::SQLiteException);
	
	sqliteDBConn.close();
}

TEST_F( SQLiteConnectionTests, testSuccessfullExecutes )
{
	std::string fileName = "SimpleSpaceSave.db";

	remove( fileName.c_str() );

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName( fileName );
	EXPECT_NO_THROW( sqliteDBConn.createDatabase() );

	EXPECT_NO_THROW( sqliteDBConn.execute( "CREATE TABLE A (fieldX INTEGER, fieldY TEXT)" ) );

	EXPECT_NO_THROW( sqliteDBConn.execute( "INSERT INTO A VALUES (1, 'text1')" ) );
	EXPECT_NO_THROW( sqliteDBConn.execute( "INSERT INTO A VALUES (2, 'text2')" ) );
	EXPECT_NO_THROW( sqliteDBConn.execute( "INSERT INTO A VALUES (3, 'text3')" ) );
	ca::SQLiteResultSet rs;
	EXPECT_NO_THROW( sqliteDBConn.executeQuery( "SELECT * FROM A", rs ) );

	rs.finalize();
	
	sqliteDBConn.close();
}

TEST_F( SQLiteConnectionTests, openExistingDB )
{
	std::string fileName = "SimpleSpaceSave.db";

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName( fileName );

	remove( fileName.c_str() );

	sqliteDBConn.createDatabase();

	EXPECT_NO_THROW( sqliteDBConn.close() );

	EXPECT_NO_THROW( sqliteDBConn.open() );
	
	//you cannot open an already opened database
	EXPECT_THROW( sqliteDBConn.open(), ca::SQLiteException );

	EXPECT_NO_THROW( sqliteDBConn.close() );

	//test harmless double close
	EXPECT_NO_THROW( sqliteDBConn.close() );

}

TEST_F( SQLiteConnectionTests, closeDBFail )
{
	std::string fileName = "SimpleSpaceSave.db";

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName(fileName);

	remove(fileName.c_str());

	sqliteDBConn.createDatabase();

	EXPECT_NO_THROW(sqliteDBConn.execute("CREATE TABLE A (fieldX INTEGER, fieldY TEXT)"));
	EXPECT_NO_THROW(sqliteDBConn.execute("INSERT INTO A VALUES (1, 'text1')"));

	ca::SQLiteResultSet rs;
	EXPECT_NO_THROW( sqliteDBConn.executeQuery( "SELECT * FROM A", rs ) );

	//not finalized IResultSet
	EXPECT_THROW(sqliteDBConn.close(), ca::SQLiteException);

	rs.finalize();
}

TEST_F( SQLiteConnectionTests, preparedStatementTest )
{
	std::string fileName = "testStatement.db";

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName(fileName);

	remove(fileName.c_str());

	sqliteDBConn.createDatabase();

	EXPECT_NO_THROW(sqliteDBConn.execute("CREATE TABLE A (fieldX INTEGER, fieldY TEXT)"));
	ca::SQLitePreparedStatement stmt;
	EXPECT_NO_THROW( sqliteDBConn.createPreparedStatement("INSERT INTO A VALUES (?, ?)", stmt) );

	std::string str("value");

	EXPECT_NO_THROW( stmt.bind( 1, 1 ));
	EXPECT_NO_THROW( stmt.bind( 2, "value" ));
	EXPECT_NO_THROW( stmt.execute() );

	EXPECT_NO_THROW( sqliteDBConn.createPreparedStatement("SELECT * FROM A WHERE fieldX =  ?", stmt) );
	EXPECT_NO_THROW( stmt.bind( 1, 1 ) );
	ca::SQLiteResultSet rs;

	EXPECT_NO_THROW( stmt.execute( rs ) );
	
	EXPECT_TRUE( rs.next() );

	EXPECT_EQ( rs.getValue( 0 ), "1" );
	EXPECT_EQ( rs.getValue( 1 ), "value" );

	//not finalized IResultSet
	EXPECT_THROW( sqliteDBConn.close(), ca::SQLiteException );
	
	stmt.finalize();

	EXPECT_NO_THROW( sqliteDBConn.close() );
}






