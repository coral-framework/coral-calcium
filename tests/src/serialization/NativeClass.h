#ifndef _NATIVE_CLASS_SERIALIZATION_
#define _NATIVE_CLASS_SERIALIZATION_

#include <string>
#include <stdio.h>

namespace serializationNative {
	class NativeClass
	{
	public:
		std::string strValue;
		double doubleValue;
		int intValue;
		char byteValue;
		
		inline bool operator == ( const NativeClass& other ) const 
		{ 
			return strcmp(strValue.c_str(), other.strValue.c_str()) == 0 &&
				doubleValue == other.doubleValue &&
				intValue == other.intValue &&
				byteValue == other.byteValue;
		}
	};
}

#endif