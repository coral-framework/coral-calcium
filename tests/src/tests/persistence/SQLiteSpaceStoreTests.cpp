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
		std::string fileName = "spaceStoreTest.db";

		remove( fileName.c_str() );

		spaceStoreObj = co::newInstance( "ca.SQLiteSpaceStore" );

		(spaceStoreObj->getService<ca::INamed>())->setName( fileName );

		spaceStore = spaceStoreObj->getService<ca::ISpaceStore>();
	}
	co::RefPtr<co::IObject> spaceStoreObj;
	ca::ISpaceStore* spaceStore;
};

TEST_F( SQLiteSpaceStoreTests, testOpsWithoutOpen )
{

	EXPECT_THROW( spaceStore->getOrAddType("type", 1), ca::IOException );
	EXPECT_THROW( spaceStore->addObject(1), ca::IOException );
	EXPECT_THROW( spaceStore->addField(1, "field", 1), ca::IOException );
	EXPECT_THROW( spaceStore->addField(1, "field2", 1), ca::IOException );

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

	EXPECT_THROW( spaceStore->getObjectType(1), ca::IOException );
}

TEST_F( SQLiteSpaceStoreTests, testChangesWithoutBegin )
{

	EXPECT_NO_THROW(spaceStore->open());

	EXPECT_THROW( spaceStore->getOrAddType("type", 1), ca::IOException );
	EXPECT_THROW( spaceStore->addObject(1), ca::IOException );
	EXPECT_THROW( spaceStore->addField(1, "field", 1), ca::IOException );
	EXPECT_THROW( spaceStore->addField(1, "field2", 1), ca::IOException );

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

	ASSERT_FALSE ( type1InsertedId == 0 );

	spaceStore->endChanges();

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
	EXPECT_NO_THROW( type1InsertedId = spaceStore->getOrAddType( "type1", 1 ) );
	EXPECT_NO_THROW( type2InsertedId = spaceStore->getOrAddType( "type2", 2 ) );

	EXPECT_THROW(spaceStore->addObject( 100 ), ca::IOException);

	co::uint32 obj1InsertedId, obj2InsertedId;
	EXPECT_NO_THROW( obj1InsertedId = spaceStore->addObject( type1InsertedId ) );
	EXPECT_NO_THROW( obj2InsertedId = spaceStore->addObject( type2InsertedId ) );

	ASSERT_FALSE ( obj1InsertedId == 0 );
	ASSERT_FALSE ( obj2InsertedId == 0 );

	EXPECT_NO_THROW( spaceStore->endChanges() );

	co::uint32 type1ConsultedId, type2ConsultedId;
	EXPECT_NO_THROW( type1ConsultedId = spaceStore->getObjectType( obj1InsertedId ) );
	EXPECT_NO_THROW( type2ConsultedId = spaceStore->getObjectType( obj2InsertedId ) );

	ASSERT_EQ( type1InsertedId, type1ConsultedId );
	ASSERT_EQ( type2InsertedId, type2ConsultedId );
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

	EXPECT_NO_THROW( field1 = spaceStore->addField( type1InsertedId, "field", coint32InsertedId ) );
	EXPECT_NO_THROW( field2 = spaceStore->addField( type1InsertedId, "field2", stringInsertedId ) );
	EXPECT_NO_THROW( field3 = spaceStore->addField( type1InsertedId, "parent", type1InsertedId ) );

	EXPECT_THROW( spaceStore->addField( 73462843, "invalid", coint32InsertedId ), ca::IOException ); //type id invalid
	EXPECT_THROW( spaceStore->addField( type1InsertedId, "invalid", 918723 ), ca::IOException ); //field type id invalid

	spaceStore->endChanges();

	ca::StoredType type;

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

	EXPECT_EQ( 0, type.fields.size() );
	EXPECT_EQ( type.typeId, coint32InsertedId );
	EXPECT_EQ( type.typeName, "co.int32" );

	spaceStore->close();
}