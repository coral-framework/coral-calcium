#include <ca/ISpaceStore.h>
#include <gtest/gtest.h>
#include <co/Coral.h>
#include <co/IObject.h>
#include <co/RefPtr.h>
#include "persistence/sqlite/SQLite.h"
#include <ca/INamed.h>
#include <co/reserved/OS.h>
#include <ca/IOException.h>
#include "ca/StoredFieldValue.h"
#include "ca/StoredType.h"


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

	EXPECT_THROW( spaceStore->getOrAddType( "type", 1 ), ca::IOException );
	EXPECT_THROW( spaceStore->addObject( 1, "type" ), ca::IOException );
	EXPECT_THROW( spaceStore->addField( 1, "field", 1, false ), ca::IOException );
	EXPECT_THROW( spaceStore->addField( 1, "field2", 1, false ), ca::IOException );

	std::vector<ca::StoredFieldValue> values;

	ca::StoredFieldValue sfv;
	sfv.fieldId = 1;
	sfv.value = "value";
	values.push_back(sfv);

	sfv.fieldId = 2;
	sfv.value = "value2";
	values.push_back(sfv);

	EXPECT_THROW( spaceStore->addValues(1, values), ca::IOException );

	ca::StoredType type;

	EXPECT_THROW( spaceStore->getType(1, type), ca::IOException );

	EXPECT_THROW( spaceStore->getRootObject(1), ca::IOException );

	EXPECT_THROW( spaceStore->getValues(1, 1, values), ca::IOException );

	std::string typeName;
	EXPECT_THROW( spaceStore->getObjectType( 1, typeName ), ca::IOException );
}

TEST_F( SQLiteSpaceStoreTests, testChangesWithoutBegin )
{

	ASSERT_NO_THROW(spaceStore->open());

	EXPECT_THROW( spaceStore->getOrAddType("type", 1), ca::IOException );
	EXPECT_THROW( spaceStore->addObject( 1, "type" ), ca::IOException );
	EXPECT_THROW( spaceStore->addField(1, "field", 1, false), ca::IOException );
	EXPECT_THROW( spaceStore->addField(1, "field2", 1, false), ca::IOException );

	std::vector<ca::StoredFieldValue> values;

	ca::StoredFieldValue sfv;
	sfv.fieldId = 1;
	sfv.value = "value";
	values.push_back(sfv);

	sfv.fieldId = 2;
	sfv.value = "value2";
	values.push_back(sfv);

	EXPECT_THROW( spaceStore->addValues(1, values), ca::IOException );

	spaceStore->close();

}

TEST_F( SQLiteSpaceStoreTests, testGetOrAddType )
{
	spaceStore->open();
	spaceStore->beginChanges();
	co::uint32 type1InsertedId;
	EXPECT_NO_THROW( type1InsertedId = spaceStore->getOrAddType( "type1", 1 ) );

	ASSERT_FALSE ( type1InsertedId == 0 );

	co::uint32 type2InsertedId;
	EXPECT_NO_THROW( type2InsertedId = spaceStore->getOrAddType( "type2", 2 ) );

	ASSERT_FALSE ( type2InsertedId == 0 );
	ASSERT_TRUE ( type1InsertedId != type2InsertedId );

	spaceStore->commitChanges();

	co::uint32 type1ConsultedId, type2ConsultedId;
	EXPECT_NO_THROW( type1ConsultedId = spaceStore->getOrAddType( "type1", 1 ) );
	EXPECT_NO_THROW( type2ConsultedId = spaceStore->getOrAddType( "type2", 2 ) );

	ASSERT_EQ( type1InsertedId, type1ConsultedId );
	ASSERT_EQ( type2InsertedId, type2ConsultedId );
	spaceStore->close();
}

TEST_F( SQLiteSpaceStoreTests, testAddObjectGetObject )
{
	spaceStore->open();
	co::uint32 type1InsertedId, type2InsertedId;
	EXPECT_NO_THROW( spaceStore->beginChanges() );

	const std::string typeNameInserted1 = "type1", typeNameInserted2 = "type2";

	EXPECT_NO_THROW( type1InsertedId = spaceStore->getOrAddType( typeNameInserted1, 1 ) );
	EXPECT_NO_THROW( type2InsertedId = spaceStore->getOrAddType( typeNameInserted2, 2 ) );

	co::uint32 obj1InsertedId, obj2InsertedId;
	EXPECT_NO_THROW( obj1InsertedId = spaceStore->addObject( type1InsertedId, typeNameInserted1 ) );
	EXPECT_NO_THROW( obj2InsertedId = spaceStore->addObject( type2InsertedId, typeNameInserted2 ) );

	ASSERT_FALSE ( obj1InsertedId == 0 );
	ASSERT_FALSE ( obj2InsertedId == 0 );
	ASSERT_TRUE ( obj1InsertedId != obj2InsertedId );

	EXPECT_THROW( spaceStore->addObject( 100, "typeInvalid" ), ca::IOException );

	EXPECT_NO_THROW( spaceStore->commitChanges() );

	EXPECT_EQ( 1, spaceStore->getLatestRevision() );

	std::string typeName1, typeName2;
	EXPECT_NO_THROW( spaceStore->getObjectType( obj1InsertedId, typeName1 ) );
	EXPECT_NO_THROW( spaceStore->getObjectType( obj2InsertedId, typeName2 ) );

	ASSERT_EQ( typeNameInserted1, typeName1 );
	ASSERT_EQ( typeNameInserted2, typeName2 );
	spaceStore->close();
}

TEST_F( SQLiteSpaceStoreTests, testTypeAndFields )
{
	spaceStore->open();
	spaceStore->beginChanges();

	co::uint32 type1InsertedId, coint32InsertedId, stringInsertedId;
	EXPECT_NO_THROW( type1InsertedId = spaceStore->getOrAddType( "type1", 1 ) );
	EXPECT_NO_THROW( coint32InsertedId = spaceStore->getOrAddType( "co.int32", 1 ) );
	EXPECT_NO_THROW( stringInsertedId = spaceStore->getOrAddType( "string", 1 ) );

	co::uint32 field1, field2, field3;

	EXPECT_NO_THROW( field1 = spaceStore->addField( type1InsertedId, "field", coint32InsertedId, false ) );
	EXPECT_NO_THROW( field2 = spaceStore->addField( type1InsertedId, "field2", stringInsertedId, false ) );
	EXPECT_NO_THROW( field3 = spaceStore->addField( type1InsertedId, "parent", type1InsertedId, false ) );

	EXPECT_THROW( spaceStore->addField( 73462843, "invalid", coint32InsertedId, false ), ca::IOException ); //type id invalid
	EXPECT_THROW( spaceStore->addField( type1InsertedId, "invalid", 918723, false ), ca::IOException ); //field type id invalid
	EXPECT_THROW( spaceStore->addField( 73462843, "invalid", 918723, false ), ca::IOException ); //both are invalid

	spaceStore->commitChanges();

	ca::StoredType type;

	EXPECT_NO_THROW( spaceStore->getType( 827456832, type ) ); //invalid id
	EXPECT_TRUE( type.typeId == 0 );
	EXPECT_TRUE( type.typeName.empty() );
	EXPECT_TRUE( type.fields.empty() );

	EXPECT_NO_THROW( spaceStore->getType( type1InsertedId, type ) );

	EXPECT_EQ( "type1", type.typeName );
	EXPECT_EQ( type1InsertedId, type.typeId );

	ASSERT_EQ( 3, type.fields.size() );

	ca::StoredField field = type.fields[0];

	EXPECT_EQ( field1, field.fieldId );
	EXPECT_EQ( "field", field.fieldName );
	EXPECT_EQ( coint32InsertedId, field.typeId );

	field = type.fields[1];

	EXPECT_EQ( field2, field.fieldId );
	EXPECT_EQ( "field2", field.fieldName );
	EXPECT_EQ( stringInsertedId, field.typeId );

	field = type.fields[2];

	EXPECT_EQ( field3, field.fieldId );
	EXPECT_EQ( "parent", field.fieldName );
	EXPECT_EQ( type1InsertedId, field.typeId );

	//test get a basic type

	EXPECT_NO_THROW( spaceStore->getType( coint32InsertedId, type ));

	ASSERT_EQ( 0, type.fields.size() );
	EXPECT_EQ( type.typeId, coint32InsertedId );
	EXPECT_EQ( type.typeName, "co.int32" );

	spaceStore->close();
}

TEST_F( SQLiteSpaceStoreTests, testAddAndGetValues )
{
	spaceStore->open();
	spaceStore->beginChanges();

	co::uint32 type1InsertedId, coint32InsertedId, stringInsertedId;
	EXPECT_NO_THROW( type1InsertedId = spaceStore->getOrAddType( "type1", 1 ) );
	EXPECT_NO_THROW( coint32InsertedId = spaceStore->getOrAddType( "co.int32", 1 ) );
	EXPECT_NO_THROW( stringInsertedId = spaceStore->getOrAddType( "string", 1 ) );

	co::uint32 field1, field2, field3;

	EXPECT_NO_THROW( field1 = spaceStore->addField( type1InsertedId, "field", coint32InsertedId, false ) );
	EXPECT_NO_THROW( field2 = spaceStore->addField( type1InsertedId, "field2", stringInsertedId, false ) );
	EXPECT_NO_THROW( field3 = spaceStore->addField( type1InsertedId, "parent", type1InsertedId, false ) );

	co::uint32 objectId;
	ASSERT_NO_THROW( objectId = spaceStore->addObject( type1InsertedId, "type1" ) );

	std::vector<ca::StoredFieldValue> values;

	ca::StoredFieldValue sfv;

	sfv.fieldId = 82745632; //invalid fieldId
	sfv.value = "invalid";
	values.push_back( sfv );

	EXPECT_THROW( spaceStore->addValues( objectId, values ), ca::IOException );
	values.clear();

	sfv.fieldId = field1;
	sfv.fieldName = "field";
	sfv.value = "1";
	
	values.push_back( sfv );

	sfv.fieldId = field2;
	sfv.fieldName = "field2";
	sfv.value = "'stringValue'";

	values.push_back( sfv );

	sfv.fieldId = field3;
	sfv.fieldName = "parent";
	sfv.value = "nil";

	values.push_back( sfv );

	EXPECT_THROW( spaceStore->addValues( 2987343, values ), ca::IOException ); //invalid object

	EXPECT_NO_THROW( spaceStore->addValues( objectId, values ) );

	spaceStore->commitChanges();

	std::vector<ca::StoredFieldValue> valuesRestored;

	ASSERT_NO_THROW( spaceStore->getValues( 287364873, spaceStore->getLatestRevision(), valuesRestored ) ); //invalid id
	EXPECT_TRUE( valuesRestored.empty() );

	ASSERT_NO_THROW( spaceStore->getValues( objectId, spaceStore->getLatestRevision(), valuesRestored ) );

	ASSERT_EQ( values.size(), valuesRestored.size() );

	for( unsigned int i = 0; i < values.size(); i++ )
	{
		EXPECT_EQ( values[i].fieldId, valuesRestored[i].fieldId );
		EXPECT_EQ( values[i].fieldName, valuesRestored[i].fieldName );
		EXPECT_EQ( values[i].value, valuesRestored[i].value );
	}

	spaceStore->close();
}

TEST_F( SQLiteSpaceStoreTests, testDiscardChanges )
{
	spaceStore->open();

	//you can't discardChanges without beginChanges
	ASSERT_THROW( spaceStore->discardChanges(), ca::IOException );

	spaceStore->beginChanges();

	co::uint32 type1InsertedId, coint32InsertedId, stringInsertedId;
	EXPECT_NO_THROW( type1InsertedId = spaceStore->getOrAddType( "type1", 1 ) );
	EXPECT_NO_THROW( coint32InsertedId = spaceStore->getOrAddType( "co.int32", 1 ) );
	EXPECT_NO_THROW( stringInsertedId = spaceStore->getOrAddType( "string", 1 ) );

	co::uint32 field1, field2, field3;

	EXPECT_NO_THROW( field1 = spaceStore->addField( type1InsertedId, "field", coint32InsertedId, false ) );
	EXPECT_NO_THROW( field2 = spaceStore->addField( type1InsertedId, "field2", stringInsertedId, false ) );
	EXPECT_NO_THROW( field3 = spaceStore->addField( type1InsertedId, "parent", type1InsertedId, false ) );

	co::uint32 objectId;
	ASSERT_NO_THROW( objectId = spaceStore->addObject( type1InsertedId, "type1" ) );

	std::vector<ca::StoredFieldValue> values;

	ca::StoredFieldValue sfv;

	sfv.fieldId = field1;
	sfv.value = "1";

	values.push_back( sfv );

	sfv.fieldId = field2;
	sfv.value = "'stringValue'";

	values.push_back( sfv );

	sfv.fieldId = field3;
	sfv.value = "nil";

	values.push_back( sfv );

	EXPECT_NO_THROW( spaceStore->addValues( objectId, values ) );

	EXPECT_NO_THROW( spaceStore->discardChanges() );

	//you can't commit after a discardChanges
	ASSERT_THROW( spaceStore->commitChanges(), ca::IOException );

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

	ca::SQLiteStatement stmtType = conn.prepare( "SELECT * FROM TYPE" );

	ca::SQLiteResult rsType = stmtType.query();

	ASSERT_FALSE( rsType.next() );

	stmtType.finalize();

	ca::SQLiteStatement stmtFields = conn.prepare( "SELECT * FROM FIELD" );

	ca::SQLiteResult rsFields = stmtFields.query();

	ASSERT_FALSE( rsFields.next() );

	stmtFields.finalize();

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

	co::uint32 type1InsertedId, coint32InsertedId, stringInsertedId;
	EXPECT_NO_THROW( type1InsertedId = spaceStore->getOrAddType( "type1", 1 ) );
	EXPECT_NO_THROW( coint32InsertedId = spaceStore->getOrAddType( "co.int32", 1 ) );
	EXPECT_NO_THROW( stringInsertedId = spaceStore->getOrAddType( "string", 1 ) );

	co::uint32 field1, field2, field3;

	EXPECT_NO_THROW( field1 = spaceStore->addField( type1InsertedId, "field", coint32InsertedId, false ) );
	EXPECT_NO_THROW( field2 = spaceStore->addField( type1InsertedId, "field2", stringInsertedId, false ) );
	EXPECT_NO_THROW( field3 = spaceStore->addField( type1InsertedId, "parent", type1InsertedId, false ) );

	spaceStore->commitChanges();

	//type definitions do not alter revision
	ASSERT_TRUE( spaceStore->getLatestRevision() == 0 );

	spaceStore->beginChanges();
	co::uint32 objectId;
	ASSERT_NO_THROW( objectId = spaceStore->addObject( type1InsertedId, "type1" ) );

	//do not increment revision until a change has been committed
	ASSERT_TRUE( spaceStore->getLatestRevision() == 0 );

	spaceStore->commitChanges();

	//now we have a new revision
	ASSERT_TRUE( spaceStore->getLatestRevision() == 1 );

	ASSERT_THROW( objectId = spaceStore->addObject( type1InsertedId, "type1" ), ca::IOException );
	ASSERT_TRUE( spaceStore->getLatestRevision() == 1 ); // revision is not updated without a begin/commit pair

	spaceStore->beginChanges();
	ASSERT_NO_THROW( objectId = spaceStore->addObject( type1InsertedId, "type1" ));
	spaceStore->commitChanges();
	ASSERT_TRUE( spaceStore->getLatestRevision() == 2 ); // another revision

	spaceStore->beginChanges();
	ASSERT_NO_THROW( spaceStore->addObject( type1InsertedId, "type1" ) );
	spaceStore->discardChanges();

	//discarded changes can't change revision
	ASSERT_TRUE( spaceStore->getLatestRevision() == 2 );

	spaceStore->close();


}