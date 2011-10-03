
#include <gtest/gtest.h>
#include <co/Coral.h>
#include <co/RefPtr.h>
#include <co/Any.h>
#include <co/IObject.h>
#include <ca/IStringSerializer.h>

class StringSerializationTests {

};

//#if 0

enum Example
{
	EXAMPLE_FIRST,
	EXAMPLE_SECOND
};

TEST( StringSerializationTests, stringDefinitionBasicTypes )
{

	co::RefPtr<co::IObject> serializerComp = co::newInstance("ca.StringSerializer");
	
	co::RefPtr<ca::IStringSerializer> serializer = serializerComp->getService<ca::IStringSerializer>();

	bool boolean = true;
	std::string expected = "true";
	
	std::string actual;
	serializer->toString(co::Any(boolean), actual);
	EXPECT_EQ(expected, actual);

	co::int8 integer8 = -127;
	expected = "-127";
	
	serializer->toString( co::Any(integer8), actual );
	EXPECT_EQ(expected, actual);

	co::uint8 uInteger8 = 127;
	expected = "127";
	
	serializer->toString( co::Any(uInteger8), actual );
	EXPECT_EQ(expected, actual);

	co::int16 integer16 = -234;
	expected = "-234";
	
	serializer->toString( co::Any(integer16), actual );
	EXPECT_EQ(expected, actual);

	co::uint16 uInteger16 = 234;
	expected = "234";
	
	serializer->toString( co::Any(uInteger16), actual );
	EXPECT_EQ(expected, actual);

	co::int32 integer32 = -1;
	expected = "-1";
	
	serializer->toString( co::Any(integer32), actual );
	EXPECT_EQ(expected, actual);

	co::uint32 uInteger32 = 1;
	expected = "1";
	
	serializer->toString( co::Any(uInteger32), actual );
	EXPECT_EQ(expected, actual);

	co::int64 integer64 = -389457987394875;
	expected = "-389457987394875";
	
	serializer->toString( co::Any(integer64), actual );
	EXPECT_EQ(expected, actual);

	co::uint64 uInteger64 = 389457987394875;
	expected = "389457987394875";
	
	serializer->toString( co::Any(uInteger64), actual );
	EXPECT_EQ(expected, actual);

	uInteger64 = 134000000;
	expected = "1.34E8";
	
	serializer->toString( co::Any(uInteger64), actual );
	EXPECT_EQ(expected, actual);

	float floatValue = 4.312300000f;
	expected = "4.3123";
	
	serializer->toString( co::Any(floatValue), actual );
	EXPECT_EQ(expected, actual);

	floatValue = 4.0f;
	expected = "4";
	
	serializer->toString(co::Any(floatValue), actual);
	EXPECT_EQ(expected, actual);

	floatValue = 0.00000123f;
	expected = "1.23E-6";
	
	serializer->toString( co::Any(floatValue), actual );
	EXPECT_EQ(expected, actual);

	double doubleValue = 4.312300000;
	expected = "4.3123";
	
	serializer->toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	doubleValue = 4.0;
	expected = "4";
	
	serializer->toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	doubleValue = 0.00000123;
	expected = "1.23E-6";
	
	serializer->toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	const std::string& stringValue = "value";
	expected = "value";
	
	serializer->toString( co::Any(&stringValue), actual );
	EXPECT_EQ( expected, actual );

	//enum
	expected = "EXAMPLE_SECOND";
	
	serializer->toString(co::Any(Example::EXAMPLE_SECOND), actual);
	EXPECT_EQ(expected, actual);

}

struct ExampleStruct 
{
	std::string name;
	co::int32 id;
	double doubleValue;
};

TEST( StringSerializationTests, stringDefinitionCompositeTypes )
{

	co::RefPtr<co::IObject> serializerComp = co::newInstance("ca.StringSerializer");
	
	co::RefPtr<ca::IStringSerializer> serializer = serializerComp->getService<ca::IStringSerializer>();

	struct ExampleStruct structValue;
	structValue.name = "nameValue";
	structValue.id = 1;
	structValue.doubleValue = 4.132;

	std::string expected = "{name='nameValue',id=1,doubleValue=4.132}";
	std::string actual;

	serializer->toString( co::Any(structValue), actual );
	EXPECT_EQ(expected, actual);

}

TEST( StringSerializationTests, stringDefinitionArray )
{
	co::RefPtr<co::IObject> serializerComp = co::newInstance("ca.StringSerializer");
	
	co::RefPtr<ca::IStringSerializer> serializer = serializerComp->getService<ca::IStringSerializer>();

	co::int8 int8Array[] = {1,-2,3};
	
	std::string expected = "{1,-2,3}";
	
	std::string actual;

	serializer->toString( co::Any(int8Array), actual );
	EXPECT_EQ(expected, actual);

	//empty array
	co::int8 emptyArray[2] = {};
	expected = "{}";

	serializer->toString( co::Any(emptyArray), actual );
	EXPECT_EQ(expected, actual);

	co::int16 int16Array[] = {1, -2, 3};
	expected = "{1,-2,3}";
	
	serializer->toString( co::Any(int16Array), actual );
	EXPECT_EQ(expected, actual);

	co::int32 int32Array[] = {123, -234, 345};
	expected = "{123,-234,345}";
	
	serializer->toString( co::Any(int32Array), actual );
	EXPECT_EQ(expected, actual);

	co::int64 int64Array[] = {321, -432, 543};
	expected = "{321,-432,543}";
	
	serializer->toString( co::Any(int64Array), actual );
	EXPECT_EQ(expected, actual);

	co::uint8 uint8Array[] = {1, 2, 3};
	expected = "{1,2,3}";
	
	serializer->toString( co::Any(uint8Array), actual );
	EXPECT_EQ(expected, actual);

	co::uint16 uint16Array[] = {1, 2, 3};
	expected = "{1,2,3}";
	
	serializer->toString( co::Any(uint16Array), actual );
	EXPECT_EQ(expected, actual);

	co::uint32 uint32Array[] = {123};
	expected = "{123}";
	
	serializer->toString( co::Any(uint32Array), actual );
	EXPECT_EQ(expected, actual);

	co::uint64 uint64Array[] = {321, 432};
	expected = "{321,432}";
	
	serializer->toString( co::Any(uint64Array), actual );
	EXPECT_EQ(expected, actual);
	
	std::string stringArray[] = {"string1", "escaped\"", "[otherScaped"};
	expected = "{'string1','escaped\"','[otherScaped'}";
	serializer->toString( co::Any(stringArray), actual );
	EXPECT_EQ(expected, actual);

	struct ExampleStruct arrayStruct[2];

	arrayStruct[0].id = 1;
	arrayStruct[0].name = "name"; 
	arrayStruct[0].doubleValue = 4.56;

	arrayStruct[1].id = 6;
	arrayStruct[1].name = "nameSecond"; 
	arrayStruct[1].doubleValue = 1.2E3;

	expected = "{{id=1,name='name',doubleValue=4.56},{id=6,name='nameSecond',doubleValue=1.2E3}}";
	serializer->toString( co::Any(arrayStruct), actual );
	EXPECT_EQ(expected, actual);

}

//#endif