
#include <gtest/gtest.h>
#include <co/Coral.h>
#include <co/RefPtr.h>
#include <co/Any.h>
#include <co/IObject.h>
#include <co/Range.h>
#include <ca/IModel.h>
#include <co/IField.h>
#include <ca/MalformedSerializedStringException.h>
#include <co/IReflector.h>
#include <serialization/BasicTypesStruct.h>
#include <serialization/NestedStruct.h>
#include <serialization/SimpleEnum.h>
#include <serialization/NativeClassCoral.h>
#include <serialization/ArrayStruct.h>
#include <serialization/TwoLevelNestedStruct.h>
#include <co/IllegalArgumentException.h>

#include "StringSerializer.h"

class StringSerializationTests {
public:

};

template< typename T >
void compareArray(T* staticArray, std::vector<T> resultArray, int size)
{
	EXPECT_EQ(size, resultArray.size());
	for(int i = 0; i < size; i++)
	{
		EXPECT_EQ(staticArray[i], resultArray[i]);
	}
}

template< typename T>
void testComplexType( ca::StringSerializer& serializer, T& complexType )
{
	std::string strOutput;
	co::Any complexAny;
	complexAny.set<const T&>( complexType );
	co::Any complexResult;

	EXPECT_NO_THROW(serializer.toString( complexAny, strOutput ));
	
	EXPECT_NO_THROW(serializer.fromString( strOutput, co::typeOf<T>::get(), complexResult ));
	EXPECT_EQ( complexType, complexResult.get<const T&>() );
}

template< typename T>
void testArrayToAndFromString( ca::StringSerializer& serializer, T* staticArray, int size)
{
	co::Any input, result;
	std::string strOutput;

	input.setArray( co::Any::AK_Range, co::typeOf<T>::get(), 0, staticArray, size );
	EXPECT_NO_THROW(serializer.toString( input, strOutput ));
	EXPECT_NO_THROW(serializer.fromString( strOutput, co::typeOf<std::vector<T>>::get(), result ));
	compareArray<T>(staticArray, result.get<std::vector<T>&>(), size);
}

TEST( StringSerializationTests, stringDefinitionBasicTypes )
{

	ca::StringSerializer serializer;

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

	float floatValue = 4.312300000f;
	expected = "4.3123";
	
	serializer.toString( co::Any(floatValue), actual );
	EXPECT_EQ(expected, actual);

	floatValue = 4.0f;
	expected = "4";
	
	serializer.toString(co::Any(floatValue), actual);
	EXPECT_EQ(expected, actual);

	floatValue = 0.00000123f;
	expected = "1.23e-006";
	
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
	expected = "1.23e-006";
	
	serializer.toString( co::Any(doubleValue), actual );
	EXPECT_EQ(expected, actual);

	doubleValue = 0.00000000823745647;
	expected = "8.23745647e-009";
	
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

	struct serialization::BasicTypesStruct structValue;
	structValue.intValue = 1;
	structValue.strValue = "name"; 
	structValue.doubleValue = 4.56;
	structValue.byteValue = 123;

	std::string expected = "{byteValue=123,doubleValue=4.56,intValue=1,strValue='name'}";
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
	expected = "{byteValue=-64,doubleValue=6.52,intValue=4386,strValue='nameNative'}";
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

	expected = "{enumValue=Three,int16Value=1234,structValue={byteValue=123,doubleValue=4.56,intValue=1,strValue='name'}}";

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

	expected = "{basicStructs={{byteValue=123,doubleValue=4.56,intValue=1,strValue='name'},"
			   "{byteValue=67,doubleValue=10.43,intValue=73246,strValue='valueSecondStruct'}},"
			   "enums={One,Two},"
			   "integers={0,2,4,6,8}}";

	structAny.set<serialization::ArrayStruct&>(arrayStruct);

	serializer.toString( structAny, actual );
	EXPECT_EQ( expected, actual );

	struct serialization::TwoLevelNestedStruct twoLevelNestedStruct;
	
	twoLevelNestedStruct.boolean = true;
	twoLevelNestedStruct.nativeClass = nativeValue;
	twoLevelNestedStruct.nested = nestedStruct;

	structAny.set<serialization::TwoLevelNestedStruct&>(twoLevelNestedStruct);

	expected = "{boolean=true,nativeClass={byteValue=-64,doubleValue=6.52,intValue=4386,strValue='nameNative'},"
			   "nested={enumValue=Three,int16Value=1234,structValue={byteValue=123,doubleValue=4.56,intValue=1,strValue='name'}}}";
	serializer.toString( structAny, actual );
	EXPECT_EQ( expected, actual );
}

TEST( StringSerializationTests, fromAndToTestCompositeTypes )
{

	ca::StringSerializer serializer;

	struct serialization::BasicTypesStruct structValue;
	structValue.intValue = 1;
	structValue.strValue = "name"; 
	structValue.doubleValue = 4.56;
	structValue.byteValue = 123;

	testComplexType<serialization::BasicTypesStruct>(serializer, structValue);

	serialization::NativeClassCoral nativeValue;
	nativeValue.intValue = 4386;
	nativeValue.strValue = "nameNative"; 
	nativeValue.doubleValue = 6.52;
	nativeValue.byteValue = -64;
	
	testComplexType<serialization::NativeClassCoral>(serializer, nativeValue);

	struct serialization::NestedStruct nestedStruct;
	nestedStruct.int16Value = 1234;
	nestedStruct.enumValue = serialization::Three;
	nestedStruct.structValue.intValue = 1;
	nestedStruct.structValue.strValue = "name"; 
	nestedStruct.structValue.doubleValue = 4.56;
	nestedStruct.structValue.byteValue = 123;

	testComplexType<serialization::NestedStruct>(serializer, nestedStruct);

	struct serialization::TwoLevelNestedStruct twoLevelNestedStruct;
	
	twoLevelNestedStruct.boolean = true;
	twoLevelNestedStruct.nativeClass = nativeValue;
	twoLevelNestedStruct.nested = nestedStruct;
	testComplexType<serialization::TwoLevelNestedStruct>(serializer, twoLevelNestedStruct);

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
	testComplexType<serialization::ArrayStruct>(serializer, arrayStruct);
	
}

TEST( StringSerializationTests, errorSituations )
{
	ca::StringSerializer serializer;

	co::Any anyNone;
	std::string actual;

	EXPECT_THROW ( serializer.toString(anyNone, actual), co::IllegalArgumentException );

	/*co::Any anyService (serializer.get());

	EXPECT_THROW ( serializer.toString(anyService, actual), co::IllegalArgumentException );

	co::RefVector<ca::StringSerializer> refVec;
	refVec.push_back(serializer);

	anyService.setArray(co::Any::AK_RefVector, co::typeOf<ca::Ica::StringSerializer>::get(), 0, &refVec);
	EXPECT_THROW ( serializer.toString(anyService, actual), co::IllegalArgumentException );*/

}

TEST( StringSerializationTests, stringDefinitionArray )
{
	ca::StringSerializer serializer;

	std::vector<co::int8> int8vec;

	co::int8 int8Array[] = {1,-2,3};

	std::string expected = "{1,-2,3}";

	std::string actual;

	co::Any::ArrayKind ak = co::Any::AK_Range;

	co::Any anyArray(co::Any::AK_Range, co::typeOf<co::int8>::get(), 0, int8Array, 3 ) ;
	
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::int16 int16Array[] = {1, -2, 3};
	expected = "{1,-2,3}";

	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::int16>::get(), 0, int16Array, 3 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::int32 int32Array[] = {123, -234, 345};
	expected = "{123,-234,345}";

	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::int32>::get(), 0, int32Array, 3 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::int64 int64Array[] = {321, -432, 543};
	expected = "{321,-432,543}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::int64>::get(), 0, int64Array, 3 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::uint8 uint8Array[] = {1, 2, 3};
	expected = "{1,2,3}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::uint8>::get(), 0, uint8Array, 3 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::uint16 uint16Array[] = {1, 2, 3};
	expected = "{1,2,3}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::uint16>::get(), 0, uint16Array, 3 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::uint32 uint32Array[] = {123};
	expected = "{123}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::uint32>::get(), 0, uint32Array, 1 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	co::uint64 uint64Array[] = {321, 432};
	expected = "{321,432}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<co::uint64>::get(), 0, uint64Array, 2 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	float floatArray[] = {5.67f, -3.12e-10f};
	expected = "{5.67,-3.12e-010}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<float>::get(), 0, floatArray, 2 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	double doubleArray[] = {-53485.67321312, 123e10};
	expected = "{-53485.67321312,1230000000000}";
	
	anyArray.setArray( co::Any::AK_Range, co::typeOf<double>::get(), 0, doubleArray, 2 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);
	
	std::string stringArray[] = {"string1", "escaped\'", "[notScaped"};
	expected = "{'string1',[=[escaped\']=],'[notScaped'}";

	anyArray.setArray( co::Any::AK_Range, co::typeOf<std::string>::get(), 0, stringArray, 3 );
	serializer.toString( anyArray, actual );
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
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	serialization::SimpleEnum enumArray[4] = {serialization::One, serialization::Three, serialization::One, serialization::Two};
	expected = "{One,Three,One,Two}";
	actual = "";
	anyArray.setArray( co::Any::AK_Range, co::typeOf<serialization::SimpleEnum>::get(), 0, enumArray, 4 );
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

	std::vector<co::int32> empty;
	anyArray.set<const std::vector<co::int32>&>(empty);

	expected = "{}";
	serializer.toString( anyArray, actual );
	EXPECT_EQ(expected, actual);

}

TEST( StringSerializationTests, fromStringPrimitiveTypesTest )
{
	ca::StringSerializer serializer;

	co::Any input;
	co::Any result;
	bool boolCaseTest = false;
	input.set<bool>(boolCaseTest);

	std::string strOutput;
	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<bool>::get(), result );
	EXPECT_EQ(boolCaseTest, result.get<bool>());

	co::int8 int8CaseTest = -83;

	input.set<co::int8>(int8CaseTest);
	co::typeOf<co::int8>::get();
	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<co::int8>::get(), result);
	EXPECT_EQ(int8CaseTest, result.get<co::int8>());

	co::uint8 uint8CaseTest = 233;
	input.set<co::uint8>(uint8CaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<co::uint8>::get(), result );
	EXPECT_EQ(uint8CaseTest, result.get<co::uint8>());

	co::int16 int16CaseTest = -5243;
	input.set<co::int16>(int16CaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<co::int16>::get(), result );
	EXPECT_EQ(int16CaseTest, result.get<co::int16>());

	co::uint16 uint16CaseTest = 5243;
	input.set<co::int16>(uint16CaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<co::uint16>::get(), result );
	EXPECT_EQ(uint16CaseTest, result.get<co::uint16>());
	
	co::int32 int32CaseTest = -7132468;
	input.set<co::int32>(int32CaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<co::int32>::get(), result );
	EXPECT_EQ(int32CaseTest, result.get<co::int32>());

	co::uint32 uint32CaseTest = 743132468;
	input.set<co::uint32>(uint32CaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<co::uint32>::get(), result );
	EXPECT_EQ(uint32CaseTest, result.get<co::uint32>());

	co::int64 int64CaseTest = -5437132468;
	input.set<co::int64>(int64CaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<co::int64>::get(), result );
	EXPECT_EQ(int64CaseTest, result.get<co::int64>());

	co::uint64 uint64CaseTest = 5437132468;
	input.set<co::uint64>(uint64CaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<co::uint64>::get(), result );
	EXPECT_EQ(uint64CaseTest, result.get<co::uint64>());

	float floatCaseTest = 67514.3223f;
	input.set<float>(floatCaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<float>::get(), result );
	EXPECT_EQ(floatCaseTest, result.get<float>());

	double doubleCaseTest = 67514.3223f;
	input.set<double>(doubleCaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<double>::get(), result );
	EXPECT_EQ(doubleCaseTest, result.get<double>());

	std::string quotesSurrounding = "withoutEscapeCharacters";
	input.set<const std::string&>(quotesSurrounding);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<std::string>::get(), result );
	std::string fromAny = result.get<const std::string&>();
	EXPECT_EQ(quotesSurrounding, fromAny);

	std::string longBracketsSurrounding = "some \'\t\\ escape characters";
	input.set<const std::string&>(longBracketsSurrounding);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<std::string>::get(), result );
	EXPECT_EQ( longBracketsSurrounding, result.get<const std::string&>() );

	serialization::SimpleEnum enumCaseTest = serialization::Two;
	input.set<serialization::SimpleEnum>(enumCaseTest);

	serializer.toString( input, strOutput );
	serializer.fromString( strOutput, co::typeOf<serialization::SimpleEnum>::get(), result );
	EXPECT_EQ( enumCaseTest, result.get<serialization::SimpleEnum>() );

}

TEST( StringSerializationTests, fromStringPrimitiveArrayTest )
{
	ca::StringSerializer serializer;

	co::int8 int8Array[] = {1,-2,3};
	testArrayToAndFromString<co::int8>(serializer, int8Array, 3);
	
	co::int16 int16Array[] = {1, -2, 3};
	testArrayToAndFromString<co::int16>(serializer, int16Array, 3);

	co::int32 int32Array[] = {123, -234, 345};
	testArrayToAndFromString<co::int32>(serializer, int32Array, 3);

	co::int64 int64Array[] = {321, -432, 543};
	testArrayToAndFromString<co::int64>(serializer, int64Array, 3);	

	co::uint8 uint8Array[] = {1, 2, 3};
	testArrayToAndFromString<co::uint8>(serializer, uint8Array, 3);	

	co::uint16 uint16Array[] = {1, 2, 3};
	testArrayToAndFromString<co::uint16>(serializer, uint16Array, 3);	
	
	co::uint32 uint32Array[] = {123};
	testArrayToAndFromString<co::uint32>(serializer, uint32Array, 1);
	
	co::uint64 uint64Array[] = {321, 432};
	testArrayToAndFromString<co::uint64>(serializer, uint64Array, 2);
	
	float floatArray[] = {5.67f, -3.12e-10f};
	testArrayToAndFromString<float>(serializer, floatArray, 2);

	double doubleArray[] = {-53485.67321312, 123e10};
	testArrayToAndFromString<double>(serializer, doubleArray, 2);
	
	std::string stringArray[] = {"not escaped", "escaped\'", "some \'\t\\ escape characters"};
	testArrayToAndFromString<std::string>(serializer, stringArray, 3);

	serialization::SimpleEnum enumArray[4] = {serialization::One, serialization::Three, serialization::One, serialization::Two};
	testArrayToAndFromString<serialization::SimpleEnum>(serializer, enumArray, 4);

	co::Any input, result;
	std::string strOutput;
	
	std::vector<co::int32> empty;
	input.set<const std::vector<co::int32>&>(empty);

	EXPECT_NO_THROW(serializer.toString( input, strOutput ));
	EXPECT_NO_THROW(serializer.fromString( strOutput, co::typeOf<std::vector<co::int32>>::get(), result ));
	EXPECT_EQ(0, result.get<const std::vector<co::int32>&>().size());

	struct serialization::BasicTypesStruct arrayStruct[2];
	
	arrayStruct[0].intValue = 1;
	arrayStruct[0].strValue = "name"; 
	arrayStruct[0].doubleValue = 4.56;
	arrayStruct[0].byteValue = 123;

	arrayStruct[1].intValue = 6;
	arrayStruct[1].strValue = "nameSecond"; 
	arrayStruct[1].doubleValue = 1.2E6;
	arrayStruct[1].byteValue = -100;

	testArrayToAndFromString<serialization::BasicTypesStruct>(serializer, arrayStruct, 2);

}

void compareAnyModelAware(co::Any& expected, co::Any& actual, co::IType* type, ca::IModel* model)
{
	co::IRecordType* recordType = static_cast<co::IRecordType*>(type);

	co::RefVector<co::IField> fields;
	model->getFields( recordType, fields );

	co::IReflector* reflect = recordType->getReflector();

	co::Any fieldValueExpected, fieldValueActual;

	for( int i = 0; i < fields.size(); i++)
	{
		reflect->getField(expected, fields[i].get(), fieldValueExpected);
		reflect->getField(actual, fields[i].get(), fieldValueActual);
		if( !fieldValueExpected.isReference() )
		{
			//array refactoring pending
			if( fieldValueExpected.getKind() != co::TK_ARRAY ) 
			{
				EXPECT_EQ( fieldValueExpected, fieldValueActual );
			}
		}
		else 
		{
			if( fieldValueExpected.getKind() == co::TK_STRING )
			{
				EXPECT_EQ( fieldValueExpected.get<const std::string&>(), fieldValueActual.get<const std::string&>());
			}
			else 
			{
				compareAnyModelAware(fieldValueExpected, fieldValueActual, fields[i]->getType(), model);
			}
			
		}
	}
}

void testComplexTypeModelAware( ca::StringSerializer& serializer, co::Any& complexAny, co::IType* type, ca::IModel* model )
{
	std::string strOutput;
	co::Any complexResult;

	EXPECT_NO_THROW( serializer.toString( complexAny, strOutput ));
	EXPECT_NO_THROW( serializer.fromString( strOutput, type, complexResult ));

	compareAnyModelAware( complexAny, complexResult, type, model );
}


TEST( StringSerializationTests, serializationModelAwareTest )
{
	co::IObject* object = co::newInstance( "ca.Model" );
	ca::IModel* model = object->getService<ca::IModel>();
	model->setName( "serialization" );

	ca::StringSerializer serializer;
	serializer.setModel( model );

	co::Any value;

	struct serialization::BasicTypesStruct structValue;
	structValue.intValue = 1;
	structValue.strValue = "name"; 
	structValue.doubleValue = 4.56;
	structValue.byteValue = 123;

	value.set<const serialization::BasicTypesStruct&>(structValue);
	
	testComplexTypeModelAware(serializer, value, co::typeOf<serialization::BasicTypesStruct>::get(), model);

	struct serialization::NestedStruct nestedStruct;
	nestedStruct.int16Value = 1234;
	nestedStruct.enumValue = serialization::Three;
	nestedStruct.structValue.intValue = 1;
	nestedStruct.structValue.strValue = "name"; 
	nestedStruct.structValue.doubleValue = 4.56;
	nestedStruct.structValue.byteValue = 123;
	
	value.set<const serialization::NestedStruct&>(nestedStruct);
	testComplexTypeModelAware(serializer, value, co::typeOf<serialization::NestedStruct>::get(), model);

	serialization::NativeClassCoral nativeValue;
	nativeValue.intValue = 4386;
	nativeValue.strValue = "nameNative"; 
	nativeValue.doubleValue = 6.52;
	nativeValue.byteValue = -64;
	
	value.set<const serialization::NativeClassCoral&>(nativeValue);
	testComplexTypeModelAware(serializer, value, co::typeOf<serialization::NativeClassCoral>::get(), model);

	struct serialization::TwoLevelNestedStruct twoLevelNestedStruct;
	
	twoLevelNestedStruct.boolean = true;
	twoLevelNestedStruct.nativeClass = nativeValue;
	twoLevelNestedStruct.nested = nestedStruct;

	value.set<const serialization::TwoLevelNestedStruct&>(twoLevelNestedStruct);
	testComplexTypeModelAware(serializer, value, co::typeOf<serialization::TwoLevelNestedStruct>::get(), model);

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
	value.set<const serialization::ArrayStruct&>(arrayStruct);
	testComplexTypeModelAware(serializer, value, co::typeOf<serialization::ArrayStruct>::get(), model);

}

TEST( StringSerializationTests, fromStringExceptions )
{
	ca::StringSerializer serializer;

	std::string malformed;

	malformed = "trup";
	co::Any result;
	co::IType* type = co::typeOf<bool>::get();
	
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "[=[escapedNotClosed\']";

	type = co::typeOf<std::string>::get();
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "'quotedNotClosed";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "'closed wrong ]=]";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "NotOnEnum";
	type = co::typeOf<serialization::SimpleEnum>::get();
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{'11','2'";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "'11','2'}";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{'11,'2'";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{'11,2";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{byteValue&123,doubleValue=4.56,intValue=1,strValue='name'}";
	type = co::typeOf<serialization::BasicTypesStruct>::get();
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{byteValue=123;doubleValue=4.56,intValue=1,strValue='name'}";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{byteValue=123,doubleValue=4.56,intValue=1,strValue='name'";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{doubleValue=4.56,byteValue=123,intValue=1,strValue='name'}";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{,doubleValue=4.56,byteValue=123,intValue=1,strValue='name'}";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "byteValue=123,doubleValue=4.56,intValue=1,strValue='name'}";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "abc";
	type = co::typeOf<double>::get();
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

	malformed = "{3.5}";
	EXPECT_THROW( serializer.fromString( malformed, type, result), ca::MalformedSerializedStringException );

}