#include <ca/ISpaceStore.h>
#include <gtest/gtest.h>
#include <co/Coral.h>
#include <co/IObject.h>
#include <co/RefPtr.h>
#include "persistence/sqlite/SQLite.h"
#include <ca/INamed.h>
#include <co/reserved/OS.h>
#include <ca/IOException.h>


class SQLiteSpaceStoreTests : public ::testing::Test
{
public:
	void SetUp()
	{
		fileName = "spaceStoreTest.db";

		remove( fileName.c_str() );

		spaceStoreObj = co::newInstance( "ca.SQLiteSpaceStore" );

		(spaceStoreObj->getService<ca::INamed>())->setName( fileName );

		spaceStore = spaceStoreObj->getService<ca::ISpaceStore>();
	}

	std::string fileName;
	co::RefPtr<co::IObject> spaceStoreObj;
	ca::ISpaceStore* spaceStore;
};

TEST_F( SQLiteSpaceStoreTests, testOpsWithoutOpen )
{

	EXPECT_THROW( spaceStore->addObject( "type" ), ca::IOException );


	std::vector<std::string> fieldNames;
	std::vector<std::string> values;

	fieldNames.push_back( "fieldName" );
	values.push_back("value");

	fieldNames.push_back( "fieldName2" );
	values.push_back("value2");

	EXPECT_THROW( spaceStore->addValues(1, fieldNames, values), ca::IOException );

	EXPECT_THROW( spaceStore->getRootObject(1), ca::IOException );

	EXPECT_THROW( spaceStore->getValues(1, 1, fieldNames, values), ca::IOException );

	std::string typeName;
	EXPECT_THROW( spaceStore->getObjectType( 1, 1, typeName ), ca::IOException );
}

TEST_F( SQLiteSpaceStoreTests, testChangesWithoutBegin )
{

	ASSERT_NO_THROW(spaceStore->open());

	EXPECT_THROW( spaceStore->addObject( "type" ), ca::IOException );
	
	std::vector<std::string> fieldNames;
	std::vector<std::string> values;

	fieldNames.push_back( "fieldName" );
	values.push_back("value");

	fieldNames.push_back( "fieldName2" );
	values.push_back("value2");

	EXPECT_THROW( spaceStore->addValues(1, fieldNames, values), ca::IOException );

	spaceStore->close();

}

TEST_F( SQLiteSpaceStoreTests, testAddObjectGetObject )
{
	spaceStore->open();
	EXPECT_NO_THROW( spaceStore->beginChanges() );

	const std::string typeNameInserted1 = "type1", typeNameInserted2 = "type2";

	co::uint32 obj1InsertedId, obj2InsertedId;
	EXPECT_NO_THROW( obj1InsertedId = spaceStore->addObject( typeNameInserted1 ) );
	EXPECT_NO_THROW( obj2InsertedId = spaceStore->addObject( typeNameInserted2 ) );

	ASSERT_FALSE ( obj1InsertedId == 0 );
	ASSERT_FALSE ( obj2InsertedId == 0 );
	ASSERT_TRUE ( obj1InsertedId != obj2InsertedId );

	//EXPECT_THROW( spaceStore->addObject( 100, "typeInvalid" ), ca::IOException );

	EXPECT_NO_THROW( spaceStore->commitChanges("") );

	EXPECT_EQ( 1, spaceStore->getLatestRevision() );

	std::string typeName1, typeName2;
	EXPECT_NO_THROW( spaceStore->getObjectType( obj1InsertedId, 1, typeName1 ) );
	EXPECT_NO_THROW( spaceStore->getObjectType( obj2InsertedId, 1, typeName2 ) );

	ASSERT_EQ( typeNameInserted1, typeName1 );
	ASSERT_EQ( typeNameInserted2, typeName2 );
	spaceStore->close();
}

TEST_F( SQLiteSpaceStoreTests, testAddAndGetValues )
{
	spaceStore->open();
	spaceStore->beginChanges();

	co::uint32 objectId;
	ASSERT_NO_THROW( objectId = spaceStore->addObject( "type1" ) );

	std::vector<std::string> fieldNames;
	std::vector<std::string> values;

	fieldNames.push_back( "field" );
	values.push_back( "1" );
	
	fieldNames.push_back( "field2" );
	values.push_back( "stringValue" );

	fieldNames.push_back( "parent" );
	values.push_back( "nil" );

	EXPECT_THROW( spaceStore->addValues( 2987343, fieldNames, values ), ca::IOException ); //invalid object

	EXPECT_NO_THROW( spaceStore->addValues( objectId, fieldNames, values ) );

	spaceStore->commitChanges("");

	std::vector<std::string> fieldNamesRestored;
	std::vector<std::string> valuesRestored;

	ASSERT_NO_THROW( spaceStore->getValues( 287364873, spaceStore->getLatestRevision(), fieldNamesRestored, valuesRestored ) ); //invalid id
	EXPECT_TRUE( valuesRestored.empty() );

	ASSERT_NO_THROW( spaceStore->getValues( objectId, spaceStore->getLatestRevision(), fieldNamesRestored, valuesRestored ) );

	ASSERT_EQ( values.size(), valuesRestored.size() );

	for( unsigned int i = 0; i < values.size(); i++ )
	{
		EXPECT_EQ( fieldNames[i], fieldNamesRestored[i] );
		EXPECT_EQ( values[i], valuesRestored[i]);
	}

	spaceStore->close();
}

TEST_F( SQLiteSpaceStoreTests, testDiscardChanges )
{
	spaceStore->open();

	//you can't discardChanges without beginChanges
	ASSERT_THROW( spaceStore->discardChanges(), ca::IOException );

	spaceStore->beginChanges();

	co::uint32 objectId;
	ASSERT_NO_THROW( objectId = spaceStore->addObject( "type1" ) );

	
	std::vector<std::string> fieldNames;
	std::vector<std::string> values;

	fieldNames.push_back( "field1" );
	values.push_back( "1" );


	fieldNames.push_back( "field2" );
	values.push_back( "stringValue" );

	fieldNames.push_back( "field3" );
	values.push_back( "nil" );

	EXPECT_NO_THROW( spaceStore->addValues( objectId, fieldNames, values ) );

	EXPECT_NO_THROW( spaceStore->discardChanges() );

	//you can't commit after a discardChanges
	ASSERT_THROW( spaceStore->commitChanges(""), ca::IOException );

	ca::SQLiteConnection conn;
	conn.open( fileName );

	ca::SQLiteStatement stmtSpace = conn.prepare( "SELECT * FROM SPACE" );

	ca::SQLiteResult rsSpace = stmtSpace.query();

	ASSERT_FALSE( rsSpace.next() );

	stmtSpace.finalize();

	ca::SQLiteStatement stmtObject = conn.prepare( "SELECT * FROM OBJECT" );

	ca::SQLiteResult rsObject = stmtObject.query();

	ASSERT_FALSE( rsObject.next() );

	stmtObject.finalize();

	ca::SQLiteStatement stmtValues = conn.prepare( "SELECT * FROM FIELD_VALUE" );

	ca::SQLiteResult rsValues = stmtValues.query();

	ASSERT_FALSE( rsValues.next() );

	stmtValues.finalize();

	conn.close();

}

TEST_F( SQLiteSpaceStoreTests, revisionNumberTests )
{
	spaceStore->open();

	ASSERT_TRUE( spaceStore->getLatestRevision() == 0 );

	spaceStore->beginChanges();

	spaceStore->commitChanges("update1");

	//type definitions do not alter revision
	ASSERT_TRUE( spaceStore->getLatestRevision() == 0 );

	spaceStore->beginChanges();
	co::uint32 objectId, otherObjectId, serviceId;
	ASSERT_NO_THROW( objectId = spaceStore->addObject( "type1" ) );
	ASSERT_NO_THROW( serviceId = spaceStore->addService( "type2", objectId ) );
	ASSERT_NO_THROW( otherObjectId = spaceStore->addObject( "type3" ) );

	ASSERT_TRUE( objectId != otherObjectId );

	std::vector<std::string> fieldNames;
	std::vector<std::string> values;

	fieldNames.push_back( "field1" );
	fieldNames.push_back( "field2" );

	values.push_back( "value1" );
	values.push_back( "value2" );

	ASSERT_NO_THROW( spaceStore->addValues( objectId, fieldNames, values ) );

	//do not increment revision until a change has been committed
	ASSERT_TRUE( spaceStore->getLatestRevision() == 0 );

	spaceStore->commitChanges("update2");

	//now we have a new revision
	ASSERT_TRUE( spaceStore->getLatestRevision() == 1 );

	ASSERT_TRUE( spaceStore->getRootObject(1) == objectId );

	std::string updateStr;
	spaceStore->getUpdates( 1, updateStr );

	ASSERT_EQ( "update2", updateStr );

	ASSERT_THROW( objectId = spaceStore->addObject( "type1" ), ca::IOException );
	ASSERT_TRUE( spaceStore->getLatestRevision() == 1 ); // revision is not updated without a begin/commit pair

	spaceStore->beginChanges();
	ASSERT_NO_THROW( spaceStore->addObject( "type1" ));
	spaceStore->commitChanges("update3");
	ASSERT_TRUE( spaceStore->getLatestRevision() == 2 ); // another revision

	spaceStore->getUpdates( 2, updateStr );
	ASSERT_EQ( "update3", updateStr );

	spaceStore->beginChanges();

	values.clear();

	values.push_back( "value3" );
	values.push_back( "value4" );

	ASSERT_NO_THROW( spaceStore->addValues( objectId, fieldNames, values ));
	spaceStore->commitChanges("update4");
	ASSERT_TRUE( spaceStore->getLatestRevision() == 3 ); // addValues also update revision

	spaceStore->beginChanges();
	ASSERT_NO_THROW( spaceStore->addObject( "type1" ) );
	spaceStore->discardChanges();

	//discarded changes can't change revision
	ASSERT_TRUE( spaceStore->getLatestRevision() == 3 );

	spaceStore->close();

}

TEST_F( SQLiteSpaceStoreTests, addGetServiceTest )
{
	spaceStore->open();

	spaceStore->beginChanges();

	co::uint32 objectId, serviceId, otherServiceId;
	ASSERT_NO_THROW( objectId = spaceStore->addObject( "type1" ) );
	ASSERT_NO_THROW( serviceId = spaceStore->addService( "type2", objectId ) );
	ASSERT_NO_THROW( otherServiceId = spaceStore->addService( "type3", objectId ) );

	EXPECT_THROW( spaceStore->addService( "type4", 82736483 ), ca::IOException );//invalid provider

	spaceStore->commitChanges("");

	co::uint32 retrievedProviderId = spaceStore->getServiceProvider( serviceId, 1 );
	co::uint32 otherRetrievedProviderId = spaceStore->getServiceProvider( otherServiceId, 1 );

	EXPECT_EQ( objectId, retrievedProviderId );
	EXPECT_EQ( objectId, otherRetrievedProviderId );

	spaceStore->close();

}