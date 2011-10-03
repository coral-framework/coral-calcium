#include <co/Any.h>
#include <co/Coral.h>

#include "StringSerializer_Base.h"

namespace ca {

	class StringSerializer : public StringSerializer_Base
	{

	public:
		StringSerializer()
		{
		
		}

		void fromString( const std::string& valueToStr, co::Any& value )
		{

			value = co::Any(1);
		}
	
		void toString( const co::Any& value, std::string& valueToStr )
		{
			valueToStr = "";
		}
	};
	CORAL_EXPORT_COMPONENT(StringSerializer, StringSerializer);
};