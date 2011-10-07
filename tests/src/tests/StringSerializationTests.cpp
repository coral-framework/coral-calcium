
#include <gtest/gtest.h>
#include <co/Coral.h>
#include <co/RefPtr.h>
#include <co/Any.h>
#include <co/IObject.h>
#include <ca/IStringSerializer.h>
#include <co/IReflector.h>
#include <serialization/BasicTypesStruct.h>
#include <serialization/NestedStruct.h>
#include <serialization/SimpleEnum.h>
#include <serialization/NativeClassCoral.h>


class StringSerializationTests {

};



TEST( StringSerializationTests, stringDefinitionBasicTypes )
{

	co::RefPtr<co::IObject> serializerComp = co::newInstance("ca.StringSerializer");
	
	co::RefPtr<ca::IStringSerializer> serializer = serializerComp->getService<ca::IStringSerializer>();

	std::string expected;
	std::string actual;

	co::int8 integer8 = -127;
	expected = "-127";

	serializer->toString( co::Any(integer8), actual );
	EXPECT_EQ(expected, actual);

	bool boolean = true;
	expected = "true";
	
	serializer->toString(co::Any(boolean), actual);
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
	expected = "134000000";
	
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
	expected = "1.23e-006";
	
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
	expected = "1.23e-006";
	
	serializer->toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	doubleValue = 0.00000000823745647;
	expected = "8.23745647e-009";
	
	serializer->toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	std::string stringValue = "value";
	co::Any anyString;
	anyString.set<const std::string&>(stringValue);
	expected = "value";
	
	serializer->toString( anyString, actual );
	EXPECT_EQ( expected, actual );
	
	expected = "Two";

	serializer->toString( co::Any( serialization::Two ), actual );
	EXPECT_EQ(expected, actual);

}

TEST( StringSerializationTests, stringDefinitionCompositeTypes )
{

	co::RefPtr<co::IObject> serializerComp = co::newInstance( "ca.StringSerializer" );	
	co::RefPtr<ca::IStringSerializer> serializer = serializerComp->getService<ca::IStringSerializer>();

	struct serialization::BasicTypesStruct structValue;
	structValue.intValue = 1;
	structValue.strValue = "name"; 
	structValue.doubleValue = 4.56;
	structValue.byteValue = 123;

	std::string expected = "{byteValue=123,doubleValue=4.56,intValue=1,strValue='name'}";
	std::string actual;

	co::Any structAny;
	structAny.set<const serialization::BasicTypesStruct&>( structValue );

	serializer->toString( structAny, actual );
	EXPECT_EQ( expected, actual );

	serialization::NativeClassCoral nativeValue;
	nativeValue.intValue = 4386;
	nativeValue.strValue = "nameNative"; 
	nativeValue.doubleValue = 6.52;
	nativeValue.byteValue = -64;
	expected = "{byteValue=-64,doubleValue=6.52,intValue=4386,strValue='nameNative'}";
	structAny.set<serialization::NativeClassCoral&>( nativeValue );

	serializer->toString( structAny, actual );
	EXPECT_EQ( expected, actual );

	struct serialization::NestedStruct nestedStruct;
	nestedStruct.int16Value = 1234;
	nestedStruct.enumValue = serialization::Three;
	nestedStruct.structValue.intValue = 1;
	nestedStruct.structValue.strValue = "name"; 
	nestedStruct.structValue.doubleValue = 4.56;
	nestedStruct.structValue.byteValue = 123;

	expected = "{enumValue=Three,int16Value=1234,structValue={byteValue=123,doubleValue=4.56,intValue=1,strValue='name'}}";

	structAny.set<const serialization::NestedStruct&>( nestedStruct );

	serializer->toString( structAny, actual );
	EXPECT_EQ( expected, actual );

}

TEST( StringSerializationTests, stringDefinitionArray )
{
	co::RefPtr<co::IObject> serializerComp = co::newInstance("ca.StringSerializer");
	
	co::RefPtr<ca::IStringSerializer> serializer = serializerComp->getService<ca::IStringSerializer>();

	std::vector<co::int8> int8vec;

	co::int8 int8Array[] = {1,-2,3};

	for(int i = 0; i < 3; i++)
	{
		int8vec.push_back((co::int8)i);
	}
	
	std::string expected = "{1,-2,3}";

	std::string actual;

	co::Any::ArrayKind ak = co::Any::AK_Range;

	co::Any anyArray(co::Any::AK_Range, co::typeOf<co::int8>::get(), 0, int8Array, 3 ) ;
	
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::int16 int16Array[] = {1, -2, 3};
	expected = "{1,-2,3}";

	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::int16>::get(), 0, int16Array, 3 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::int32 int32Array[] = {123, -234, 345};
	expected = "{123,-234,345}";

	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::int32>::get(), 0, int32Array, 3 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::int64 int64Array[] = {321, -432, 543};
	expected = "{321,-432,543}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::int64>::get(), 0, int64Array, 3 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::uint8 uint8Array[] = {1, 2, 3};
	expected = "{1,2,3}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::uint8>::get(), 0, uint8Array, 3 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::uint16 uint16Array[] = {1, 2, 3};
	expected = "{1,2,3}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::uint16>::get(), 0, uint16Array, 3 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::uint32 uint32Array[] = {123};
	expected = "{123}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::uint32>::get(), 0, uint32Array, 1 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::uint64 uint64Array[] = {321, 432};
	expected = "{321,432}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::uint64>::get(), 0, uint64Array, 2 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	float floatArray[] = {5.67f, -3.12e-10f};
	expected = "{5.67,-3.12e-010}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<float>::get(), 0, floatArray, 2 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	double doubleArray[] = {-53485.67321312, 123e10};
	expected = "{-53485.67321312,1230000000000}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<double>::get(), 0, doubleArray, 2 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);
	
	std::string stringArray[] = {"string1", "escaped\"", "[otherScaped"};
	expected = "{'string1','escaped\"','[otherScaped'}";

	anyArray.setArray( co::Any::AK_Range, co::typeOf<std::string>::get(), 0, stringArray, 3 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	struct serialization::BasicTypesStruct arrayStruct[2];

	arrayStruct[0].intValue = 1;
	arrayStruct[0].strValue = "name"; 
	arrayStruct[0].doubleValue = 4.56;
	arrayStruct[0].byteValue = 123;

	arrayStruct[1].intValue = 6;
	arrayStruct[1].strValue = "nameSecond"; 
	arrayStruct[1].doubleValue = 1.2E6;
	arrayStruct[1].byteValue = -100;

	expected = "{{byteValue=123,doubleValue=4.56,intValue=1,strValue='name'},{byteValue=-100,doubleValue=1200000,intValue=6,strValue='nameSecond'}}";

	anyArray.setArray( co::Any::AK_Range, co::typeOf<struct serialization::BasicTypesStruct>::get(), 0, arrayStruct, 2 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	serialization::SimpleEnum enumArray[4] = {serialization::One, serialization::Three, serialization::One, serialization::Two};
	expected = "{One,Three,One,Two}";
	actual = "";
	anyArray.setArray( co::Any::AK_Range, co::typeOf<serialization::SimpleEnum>::get(), 0, enumArray, 4 );
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	std::vector<co::int32> empty;
	anyArray.set<const std::vector<co::int32>&>(empty);

	expected = "{}";
	serializer->toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

}
