#include "NativeClassCoral_Adapter.h"
#include <string>
using namespace std;

namespace serialization {

int NativeClassCoral_Adapter::getIntValue( serializationNative::NativeClass& nativeClass )
{
	return nativeClass.intValue;
}

double NativeClassCoral_Adapter::getDoubleValue( serializationNative::NativeClass& nativeClass )
{
	return nativeClass.doubleValue;
}

const std::string& NativeClassCoral_Adapter::getStrValue( serializationNative::NativeClass& nativeClass )
{
	return nativeClass.strValue;
}

char NativeClassCoral_Adapter::getByteValue( serializationNative::NativeClass& nativeClass )
{
	return nativeClass.byteValue;
}

void NativeClassCoral_Adapter::setByteValue(serializationNative::NativeClass& nativeClass, co::int8 byte)
{
	nativeClass.byteValue = (char)byte;
}

void NativeClassCoral_Adapter::setDoubleValue(serializationNative::NativeClass& nativeClass, double doubleValue)
{
	nativeClass.doubleValue = doubleValue;
}

void NativeClassCoral_Adapter::setIntValue(serializationNative::NativeClass& nativeClass, co::int32 intValue)
{
	nativeClass.intValue = (int)intValue;
}

void NativeClassCoral_Adapter::setStrValue(serializationNative::NativeClass& nativeClass, const std::string& strValue)
{
	nativeClass.strValue = strValue;
}

}