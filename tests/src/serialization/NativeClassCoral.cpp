#include "NativeClass.h"
#include "NativeClassCoral_Adapter.h"
#include <string>

namespace serialization {

co::int32 NativeClassCoral_Adapter::getIntValue( serialization::NativeClassCoral& nativeClass )
{
	return nativeClass.intValue;
}

double NativeClassCoral_Adapter::getDoubleValue( serialization::NativeClassCoral& nativeClass )
{
	return nativeClass.doubleValue;
}

std::string NativeClassCoral_Adapter::getStrValue( serialization::NativeClassCoral& nativeClass )
{
	return nativeClass.strValue;
}

char NativeClassCoral_Adapter::getByteValue( serialization::NativeClassCoral& nativeClass )
{
	return nativeClass.byteValue;
}

void NativeClassCoral_Adapter::setByteValue(serialization::NativeClassCoral& nativeClass, co::int8 byte)
{
	nativeClass.byteValue = (char)byte;
}

void NativeClassCoral_Adapter::setDoubleValue(serialization::NativeClassCoral& nativeClass, double doubleValue)
{
	nativeClass.doubleValue = doubleValue;
}

void NativeClassCoral_Adapter::setIntValue(serialization::NativeClassCoral& nativeClass, co::int32 intValue)
{
	nativeClass.intValue = (int)intValue;
}

void NativeClassCoral_Adapter::setStrValue(serialization::NativeClassCoral& nativeClass, const std::string& strValue)
{
	nativeClass.strValue = strValue;
}

}