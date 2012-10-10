#include "persistence/StringSerializer.h"

#include <gtest/gtest.h>
#include <co/Coral.h>
#include <co/RefPtr.h>
#include <co/IllegalArgumentException.h>
#include <co/IObject.h>

#include <ca/IModel.h>
#include <ca/FormatException.h>

#include <serialization/BasicTypesStruct.h>
#include <serialization/NestedStruct.h>
#include <serialization/SimpleEnum.h>
#include <serialization/NativeClassCoral.h>
#include <serialization/ArrayStruct.h>
#include <serialization/TwoLevelNestedStruct.h>


TEST( StringSerializationTests, stringDefinitionBasicTypes )
{
	ca::StringSerializer serializer;

	co::IObject* modelObj = co::newInstance( "ca.Model" );
	co::RefPtr<ca::IModel> model = modelObj->getService<ca::IModel>();
	model->setName( "serialization" );
	serializer.setModel( model.get() );

	std::string expected;
	std::string actual;

	co::int8 integer8 = -127;
	expected = "-127";

	serializer.toString( co::Any(integer8), actual );
	EXPECT_EQ(expected, actual);

	bool boolean = true;
	expected = "true";
	
	serializer.toString(co::Any(boolean), actual);
	EXPECT_EQ(expected, actual);

	co::uint8 uInteger8 = 127;
	expected = "127";

	serializer.toString( co::Any(uInteger8), actual );
	EXPECT_EQ(expected, actual);

	co::int16 integer16 = -234;
	expected = "-234";
	
	serializer.toString( co::Any(integer16), actual );
	EXPECT_EQ(expected, actual);

	co::uint16 uInteger16 = 234;
	expected = "234";
	
	serializer.toString( co::Any(uInteger16), actual );
	EXPECT_EQ(expected, actual);

	co::int32 integer32 = -1;
	expected = "-1";
	
	serializer.toString( co::Any(integer32), actual );
	EXPECT_EQ(expected, actual);

	co::uint32 uInteger32 = 1;
	expected = "1";
	
	serializer.toString( co::Any(uInteger32), actual );
	EXPECT_EQ(expected, actual);

	co::int64 integer64 = -389457987394875;
	expected = "-389457987394875";
	
	serializer.toString( co::Any(integer64), actual );
	EXPECT_EQ(expected, actual);

	co::uint64 uInteger64 = 389457987394875;
	expected = "389457987394875";
	
	serializer.toString( co::Any(uInteger64), actual );
	EXPECT_EQ(expected, actual);

	uInteger64 = 134000000;
	expected = "134000000";
	
	serializer.toString( co::Any(uInteger64), actual );
	EXPECT_EQ(expected, actual);

	float floatValue = 4.3125f;
	expected = "4.3125";
	
	serializer.toString( co::Any(floatValue), actual );
	EXPECT_EQ(expected, actual);

	floatValue = 4.0f;
	expected = "4";
	
	serializer.toString(co::Any(floatValue), actual);
	EXPECT_EQ(expected, actual);

	floatValue = 0.0000512f;
	expected = "5.12000005983282e-05";
	
	serializer.toString( co::Any(floatValue), actual );
	EXPECT_EQ(expected, actual);

	double doubleValue = 4.312300000;
	expected = "4.3123";
	
	serializer.toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	doubleValue = 4.0;
	expected = "4";
	
	serializer.toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	doubleValue = 0.00000123;
	expected = "1.23e-06";
	
	serializer.toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	doubleValue = 0.00000000823745647;
	expected = "8.23745647e-09";
	
	serializer.toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	std::string stringValue = "value";
	co::Any anyString;
	anyString.set<const std::string&>(stringValue);
	expected = "'value'";
	
	serializer.toString( anyString, actual );
	EXPECT_EQ( expected, actual );
	
	expected = "Two";

	serializer.toString( co::Any( serialization::Two ), actual );
	EXPECT_EQ(expected, actual);

}

TEST( StringSerializationTests, stringDefinitionCompositeTypes )
{

	ca::StringSerializer serializer;
	co::IObject* modelObj = co::newInstance( "ca.Model" );
	co::RefPtr<ca::IModel> model = modelObj->getService<ca::IModel>();
	model->setName( "serialization" );
	serializer.setModel( model.get() );

	serialization::BasicTypesStruct structValue;
	structValue.intValue = 1;
	structValue.strValue = "name"; 
	structValue.doubleValue = 4.56;
	structValue.byteValue = 123;

	std::string expected = "{byteValue=123,intValue=1,strValue='name'}";
	std::string actual;

	co::Any structAny;
	structAny.set<const serialization::BasicTypesStruct&>( structValue );

	serializer.toString( structAny, actual );
	EXPECT_EQ( expected, actual );

	serialization::NativeClassCoral nativeValue;
	nativeValue.intValue = 4386;
	nativeValue.strValue = "nameNative"; 
	nativeValue.doubleValue = 6.52;
	nativeValue.byteValue = -64;
	expected = "{byteValue=-64,doubleValue=6.52,intValue=4386}";
	structAny.set<serialization::NativeClassCoral&>( nativeValue );

	serializer.toString( structAny, actual );
	EXPECT_EQ( expected, actual );

	struct serialization::NestedStruct nestedStruct;
	nestedStruct.int16Value = 1234;
	nestedStruct.enumValue = serialization::Three;
	nestedStruct.structValue.intValue = 1;
	nestedStruct.structValue.strValue = "name"; 
	nestedStruct.structValue.doubleValue = 4.56;
	nestedStruct.structValue.byteValue = 123;

	expected = "{int16Value=1234,structValue={byteValue=123,intValue=1,strValue='name'}}";

	structAny.set<const serialization::NestedStruct&>( nestedStruct );

	serializer.toString( structAny, actual );
	EXPECT_EQ( expected, actual );

	struct serialization::ArrayStruct arrayStruct;
	arrayStruct.enums.push_back(serialization::One);
	arrayStruct.enums.push_back(serialization::Two);

	for(int i = 0; i < 5; i++)
	{
		arrayStruct.integers.push_back(i*2);
	}

	arrayStruct.basicStructs.push_back( structValue );

	structValue.byteValue = 67;
	structValue.doubleValue = 10.43;
	structValue.intValue = 73246;
	structValue.strValue = "valueSecondStruct";

	arrayStruct.basicStructs.push_back( structValue );

	expected = "{basicStructs={{byteValue=123,intValue=1,strValue='name'},"
			   "{byteValue=67,intValue=73246,strValue='valueSecondStruct'}},"
			   "enums={One,Two}}";

	structAny.set<serialization::ArrayStruct&>(arrayStruct);

	serializer.toString( structAny, actual );
	EXPECT_EQ( expected, actual );

	struct serialization::TwoLevelNestedStruct twoLevelNestedStruct;
	
	twoLevelNestedStruct.boolean = true;
	twoLevelNestedStruct.nativeClass = nativeValue;
	twoLevelNestedStruct.nested = nestedStruct;

	structAny.set<serialization::TwoLevelNestedStruct&>(twoLevelNestedStruct);

	expected = "{boolean=true,nativeClass={byteValue=-64,doubleValue=6.52,intValue=4386},"
			   "nested={int16Value=1234,structValue={byteValue=123,intValue=1,strValue='name'}}}";
	serializer.toString( structAny, actual );
	EXPECT_EQ( expected, actual );
}

TEST( StringSerializationTests, stringDefinitionArray )
{
	ca::StringSerializer serializer;

	co::RefPtr<co::IObject> modelObj = co::newInstance( "ca.Model" );
	ca::IModel* model = modelObj->getService<ca::IModel>();
	model->setName( "serialization" );
	serializer.setModel( model );
	
	std::string actual;

	co::int8 int8Array[] = { 1, -2, 3 };
	serializer.toString( int8Array, actual );
	EXPECT_EQ( "{1,-2,3}", actual );

	co::int16 int16Array[] = { 1, -2, 3 };
	serializer.toString( int16Array, actual );
	EXPECT_EQ( "{1,-2,3}", actual );

	co::int32 int32Array[] = { 123, -234, 345 };
	serializer.toString( int32Array, actual );
	EXPECT_EQ( "{123,-234,345}", actual );

	co::int64 int64Array[] = { 321, -432, 543 };
	serializer.toString( int64Array, actual );
	EXPECT_EQ( "{321,-432,543}", actual );

	co::uint8 uint8Array[] = { 1, 2, 3 };
	serializer.toString( uint8Array, actual );
	EXPECT_EQ( "{1,2,3}", actual );

	co::uint16 uint16Array[] = {1, 2, 3};
	serializer.toString( uint16Array, actual );
	EXPECT_EQ( "{1,2,3}", actual );

	co::uint32 uint32Array[] = { 123 };
	serializer.toString( uint32Array, actual );
	EXPECT_EQ( "{123}", actual );

	co::uint64 uint64Array[] = { 321, 432 };
	serializer.toString( uint64Array, actual );
	EXPECT_EQ( "{321,432}", actual );

	float floatArray[] = { 6.25f, -3.35e-10f };
	serializer.toString( floatArray, actual );
	EXPECT_EQ( "{6.25,-3.34999999962449e-10}", actual );

	double doubleArray[] = { -53485.67321312, 123e10 };
	serializer.toString( doubleArray, actual );
	EXPECT_EQ( "{-53485.67321312,1230000000000}", actual );

	std::string stringArray[] = { "string1", "escaped\'", "[notScaped" };
	serializer.toString( stringArray, actual );
	EXPECT_EQ( "{'string1',[=[escaped\']=],'[notScaped'}", actual );

	serialization::BasicTypesStruct arrayStruct[2];

	arrayStruct[0].intValue = 1;
	arrayStruct[0].strValue = "name"; 
	arrayStruct[0].doubleValue = 4.56;
	arrayStruct[0].byteValue = 123;

	arrayStruct[1].intValue = 6;
	arrayStruct[1].strValue = "nameSecond"; 
	arrayStruct[1].doubleValue = 1.2E6;
	arrayStruct[1].byteValue = -100;

	serializer.toString( arrayStruct, actual );
	EXPECT_EQ( "{{byteValue=123,intValue=1,strValue='name'},{byteValue=-100,intValue=6,strValue='nameSecond'}}", actual );

	serialization::SimpleEnum enumArray[] = {
		serialization::One,
		serialization::Three,
		serialization::One,
		serialization::Two
	};

	serializer.toString( enumArray, actual );
	EXPECT_EQ( "{One,Three,One,Two}", actual );

	std::vector<co::int32> empty;
	serializer.toString( empty, actual );
	EXPECT_EQ( "{}", actual );

}
