#ifndef _NATIVE_CLASS_SERIALIZATION_
#define _NATIVE_CLASS_SERIALIZATION_

#include <string>

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
			return (strValue == other.strValue) &&
				doubleValue == other.doubleValue &&
				intValue == other.intValue &&
				byteValue == other.byteValue;
		}
	};
}

#endif