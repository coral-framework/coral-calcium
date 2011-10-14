#include <string>
using namespace std;

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

