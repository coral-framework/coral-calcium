#include "ERMSpace.h"

#include <ca/INamed.h>
#include <ca/IDBConnection.h>
#include <ca/DBException.h>
#include <ca/IResultSet.h>
#include <co/Coral.h>
#include <gtest/gtest.h>
#include <cstdio>

class DBConnectionTest : public ERMSpace {
};



TEST_F( DBConnectionTest, testOpenNonExistingDB )
{
	std::string fileName = "SimpleSpaceSave.db";

	co::RefPtr<co::IObject> dbObj = co::newInstance( "ca.SQLiteDBConnection" );
	dbObj->getService<ca::INamed>()->setName(fileName);
	
	co::RefPtr<ca::IDBConnection> sqliteDBConn = dbObj->getService<ca::IDBConnection>();

	remove(fileName.c_str());

	EXPECT_THROW(sqliteDBConn->open(), ca::DBException);
}

TEST_F( DBConnectionTest, testExecuteWithoutOpen )
{

	std::string fileName = "SimpleSpaceSave.db";

	co::RefPtr<co::IObject> dbObj = co::newInstance( "ca.SQLiteDBConnection" );
	dbObj->getService<ca::INamed>()->setName(fileName);

	co::RefPtr<ca::IDBConnection> sqliteDBConn = dbObj->getService<ca::IDBConnection>();
	EXPECT_THROW(sqliteDBConn->execute("CREATE TABLE A (fieldX INTEGER)"), ca::DBException);
	EXPECT_THROW(sqliteDBConn->executeQuery("SELECT * FROM A"), ca::DBException);
}

TEST_F( DBConnectionTest, testCreateDatabase )
{
	std::string fileName = "SimpleSpaceSave.db";

	remove(fileName.c_str());

	co::RefPtr<co::IObject> dbObj = co::newInstance( "ca.SQLiteDBConnection" );
	dbObj->getService<ca::INamed>()->setName(fileName);

	co::RefPtr<ca::IDBConnection> sqliteDBConn = dbObj->getService<ca::IDBConnection>();

	EXPECT_NO_THROW(sqliteDBConn->createDatabase());
	FILE* fileDB = fopen(fileName.c_str(), "r");
	EXPECT_TRUE(fileDB != 0);
	fclose(fileDB);
	
	//attempt to create again the database
	EXPECT_THROW(sqliteDBConn->createDatabase(), ca::DBException);
	
	sqliteDBConn->close();
}

TEST_F( DBConnectionTest, testSuccessfullExecutes )
{
	std::string fileName = "SimpleSpaceSave.db";

	remove(fileName.c_str());

	co::RefPtr<co::IObject> dbObj = co::newInstance( "ca.SQLiteDBConnection" );
	dbObj->getService<ca::INamed>()->setName(fileName);

	co::RefPtr<ca::IDBConnection> sqliteDBConn = dbObj->getService<ca::IDBConnection>();
	EXPECT_NO_THROW(sqliteDBConn->createDatabase());

	EXPECT_NO_THROW(sqliteDBConn->execute("CREATE TABLE A (fieldX INTEGER, fieldY TEXT)"));

	EXPECT_NO_THROW(sqliteDBConn->execute("INSERT INTO A VALUES (1, 'text1')"));
	EXPECT_NO_THROW(sqliteDBConn->execute("INSERT INTO A VALUES (2, 'text2')"));
	EXPECT_NO_THROW(sqliteDBConn->execute("INSERT INTO A VALUES (3, 'text3')"));
	co::RefPtr<ca::IResultSet> rs;
	EXPECT_NO_THROW((rs = sqliteDBConn->executeQuery("SELECT * FROM A")));

	EXPECT_TRUE(rs.isValid());

	rs->finalize();
	sqliteDBConn->close();
}

TEST_F( DBConnectionTest, openExistingDB )
{
	std::string fileName = "SimpleSpaceSave.db";

	co::RefPtr<co::IObject> dbObj = co::newInstance( "ca.SQLiteDBConnection" );
	dbObj->getService<ca::INamed>()->setName(fileName);

	remove(fileName.c_str());

	co::RefPtr<ca::IDBConnection> sqliteDBConn = dbObj->getService<ca::IDBConnection>();
	sqliteDBConn->createDatabase();

	EXPECT_NO_THROW(sqliteDBConn->close());

	EXPECT_NO_THROW(sqliteDBConn->open());
	
	//you cannot open an already opened database
	EXPECT_THROW(sqliteDBConn->open(), ca::DBException);

	EXPECT_NO_THROW(sqliteDBConn->close());

	//test harmless double close
	EXPECT_NO_THROW(sqliteDBConn->close());

}

TEST_F( DBConnectionTest, closeDBFail )
{
	std::string fileName = "SimpleSpaceSave.db";

	co::RefPtr<co::IObject> dbObj = co::newInstance( "ca.SQLiteDBConnection" );
	dbObj->getService<ca::INamed>()->setName(fileName);

	remove(fileName.c_str());

	co::RefPtr<ca::IDBConnection> sqliteDBConn = dbObj->getService<ca::IDBConnection>();
	sqliteDBConn->createDatabase();

	EXPECT_NO_THROW(sqliteDBConn->execute("CREATE TABLE A (fieldX INTEGER, fieldY TEXT)"));
	EXPECT_NO_THROW(sqliteDBConn->execute("INSERT INTO A VALUES (1, 'text1')"));

	co::RefPtr<ca::IResultSet> rs;
	EXPECT_NO_THROW((rs = sqliteDBConn->executeQuery("SELECT * FROM A")));

	//not finalized IResultSet
	EXPECT_THROW(sqliteDBConn->close(), ca::DBException);

}








