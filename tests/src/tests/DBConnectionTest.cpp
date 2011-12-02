#include "ERMSpace.h"

#include <ca/INamed.h>
#include "SQLiteConnection.h"
#include "SQLiteResultSet.h"
#include "DBException.h"
#include <co/Coral.h>
#include <gtest/gtest.h>
#include <cstdio>

class DBConnectionTest : public ERMSpace {
};



TEST_F( DBConnectionTest, testOpenNonExistingDB )
{
	std::string fileName = "SimpleSpaceSave.db";

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName(fileName);

	remove(fileName.c_str());

	EXPECT_THROW(sqliteDBConn.open(), ca::DBException);
}

TEST_F( DBConnectionTest, testExecuteWithoutOpen )
{

	std::string fileName = "SimpleSpaceSave.db";

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName(fileName);

	ca::SQLiteResultSet rs;
	
	EXPECT_THROW( sqliteDBConn.execute("CREATE TABLE A (fieldX INTEGER)"), ca::DBException );
	EXPECT_THROW( sqliteDBConn.executeQuery("SELECT * FROM A", rs), ca::DBException );
}

TEST_F( DBConnectionTest, testCreateDatabase )
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
	EXPECT_THROW(sqliteDBConn.createDatabase(), ca::DBException);
	
	sqliteDBConn.close();
}

TEST_F( DBConnectionTest, testSuccessfullExecutes )
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

TEST_F( DBConnectionTest, openExistingDB )
{
	std::string fileName = "SimpleSpaceSave.db";

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName( fileName );

	remove( fileName.c_str() );

	sqliteDBConn.createDatabase();

	EXPECT_NO_THROW( sqliteDBConn.close() );

	EXPECT_NO_THROW( sqliteDBConn.open() );
	
	//you cannot open an already opened database
	EXPECT_THROW( sqliteDBConn.open(), ca::DBException );

	EXPECT_NO_THROW( sqliteDBConn.close() );

	//test harmless double close
	EXPECT_NO_THROW( sqliteDBConn.close() );

}

TEST_F( DBConnectionTest, closeDBFail )
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
	EXPECT_THROW(sqliteDBConn.close(), ca::DBException);

	rs.finalize();
}

TEST_F( DBConnectionTest, preparedStatementTest )
{
	std::string fileName = "testStatement.db";

	ca::SQLiteConnection sqliteDBConn;

	sqliteDBConn.setFileName(fileName);

	remove(fileName.c_str());

	sqliteDBConn.createDatabase();

	EXPECT_NO_THROW(sqliteDBConn.execute("CREATE TABLE A (fieldX INTEGER, fieldY TEXT)"));
	ca::SQLitePreparedStatement stmt;
	EXPECT_NO_THROW( sqliteDBConn.createPreparedStatement("INSERT INTO A VALUES (?, ?)", stmt) );

	EXPECT_NO_THROW( stmt.setInt( 1, 1 ));
	EXPECT_NO_THROW( stmt.setString( 2, "value" ));
	EXPECT_NO_THROW( stmt.execute() );

	EXPECT_NO_THROW( sqliteDBConn.createPreparedStatement("SELECT * FROM A WHERE fieldX =  ?", stmt) );
	EXPECT_NO_THROW( stmt.setInt( 1, 1 ) );
	ca::SQLiteResultSet rs;

	EXPECT_NO_THROW( stmt.execute( rs ) );
	
	EXPECT_TRUE( rs.next() );

	EXPECT_EQ( rs.getValue( 0 ), "1" );
	EXPECT_EQ( rs.getValue( 1 ), "value" );

	//not finalized IResultSet
	EXPECT_THROW( sqliteDBConn.close(), ca::DBException );
	
	stmt.finalize();

	EXPECT_NO_THROW( sqliteDBConn.close() );
}






