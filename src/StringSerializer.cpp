#include <co/Any.h>
#include <co/Coral.h>
#include <co/IType.h>
#include <cstdio>
#include <sstream>
#include <co/IClassType.h>
#include <co/IField.h>
#include <co/IReflector.h>
#include <co/IllegalArgumentException.h>
#include <limits>
using namespace std;

#include "StringSerializer_Base.h"

#define number2str(s,n) sprintf((s), "%.14g", (n))

namespace ca {

	class StringSerializer : public StringSerializer_Base
	{

	public:
		StringSerializer()
		{
		
		}

		void fromString( const std::string& valueToStr, co::Any& value )
		{
			
		}
	
		void toString( const co::Any& value, std::string& valueToStr )
		{
			if( value.getKind() == co::TK_NONE )
			{
				throw co::IllegalArgumentException("Invalid value");
			}

			stringstream valueStream;
			toStream(value, valueStream, false);
			valueStream >> valueToStr;
		}

	private:

		void toStream(const co::Any& value, stringstream& ss, bool surroundStringQuotes)
		{
			co::IType* type = value.getType();

			if(type == NULL)
			{
				getBasicTypeString(value, ss, surroundStringQuotes);
			}
			else
			{
				co::TypeKind kind = value.getKind();
				if( kind == co::TK_STRUCT || kind == co::TK_NATIVECLASS )
				{
					getComplexTypeString( value, ss, type );
				}
				if( kind == co::TK_ARRAY )
				{
					getArrayString( value, ss, type );
				}
				if( kind == co::TK_ENUM )
				{
					getEnumString( value, ss, type );
				}
			}
		}

		void getEnumString(const co::Any& value, stringstream& ss, co::IType* type)
		{
			co::IEnum* enumType = static_cast<co::IEnum*>(type);

			co::int8 enumAsint8 = value.get<co::int8>();

			ss << enumType->getIdentifiers()[enumAsint8];
		}

		void getComplexTypeString(const co::Any& value, stringstream& ss, co::IType* type)
		{
			co::Range<co::IField* const> fields = (static_cast<co::IClassType*>(type))->getFields();
			co::IReflector * ref = type->getReflector();
			co::Any fieldValue;
			std::string fieldValueStr;
			ss << "{";
			while(!fields.isEmpty())
			{
				co::IField* field = fields.getFirst();

				ss << field->getName() << "=";

				ref->getField(value, field, fieldValue);
				
				toStream(fieldValue, ss, true);
				fields.popFirst();
				if(!fields.isEmpty())
				{
					ss << ",";
				}
			}
			ss << "}";
		}

		void getArrayString(const co::Any& value, stringstream& ss, co::IType* type)
		{
			ss << "{";

			int size = getArraySize(value);
			co::Any arrayElement;
			for( int i = 0; i < size; i++ )
			{
				 getArrayElement(value, i, arrayElement);
				 toStream(arrayElement, ss, true);
				 if( i < size-1)
				 {
					ss << ",";
				 }
			}

			ss << "}";

		}

		void getBasicTypeString(const co::Any& value, stringstream& ss, bool surroundStringQuotes)
		{
			co::TypeKind kind = value.getKind();
			char strNumber[32];
			switch(kind)
			{
			case co::TK_BOOLEAN:
				if(value.get<bool>())
				{
					ss << "true";
				}
				else
				{
					ss << "false";
				}
			break;
			case co::TK_INT8:
				sprintf(strNumber, "%i", value.get<co::int8>());
				ss << strNumber;
			break;
			case co::TK_UINT8:
				sprintf(strNumber, "%i", value.get<co::uint8>());
				ss << strNumber;
			break;
			case co::TK_INT16:
				sprintf(strNumber, "%i", value.get<co::int16>());
				ss << strNumber;
			break;
			case co::TK_UINT16:
				sprintf(strNumber, "%i", value.get<co::uint16>());
				ss << strNumber;
			break;
			case co::TK_INT32:
				ss.precision(numeric_limits<long int>::digits10 + 1);
				ss << value.get<co::int32>();
			break;
			case co::TK_UINT32:
				ss.precision(numeric_limits<unsigned long int>::digits10 + 1);
				ss << value.get<co::uint32>();
			break;
			case co::TK_INT64:

				ss.precision(numeric_limits<long long int>::digits10 + 1);
				ss << value.get<co::int64>();
			break;
			case co::TK_UINT64:
				ss.precision(numeric_limits<unsigned long long int>::digits10 + 1);
				ss << value.get<co::uint64>();
			break;
			case co::TK_FLOAT:
				ss.precision(numeric_limits<float>::digits10 + 1);
				ss << value.get<float>();
			break;
			case co::TK_DOUBLE:
				ss.precision(numeric_limits<double>::digits10 + 1);
				ss << value.get<double>();
			break;
			case co::TK_STRING:
				escapeLuaString(value.get<const std::string&>(), ss, surroundStringQuotes);
			break;

			}
		}

		co::uint32 getArraySize( const co::Any& array )
		{
			assert( array.getKind() == co::TK_ARRAY );

			const co::Any::State& s = array.getState();
			if( s.arrayKind == co::Any::State::AK_Range )
				return s.arraySize;

			co::Any::PseudoVector& pv = *reinterpret_cast<co::Any::PseudoVector*>( s.data.ptr );
			return static_cast<co::uint32>( pv.size() ) / array.getType()->getReflector()->getSize();
		}

		co::uint8* getArrayPtr( const co::Any& array )
		{
			assert( array.getKind() == co::TK_ARRAY );

			const co::Any::State& s = array.getState();
			if( s.arrayKind == co::Any::State::AK_Range )
				return reinterpret_cast<co::uint8*>( s.data.ptr );

			co::Any::PseudoVector& pv = *reinterpret_cast<co::Any::PseudoVector*>( s.data.ptr );
			return &pv[0];
		}

		void getArrayElement( const co::Any& array, co::uint32 index, co::Any& element )
		{
			co::IType* elementType = array.getType();
			co::uint32 elementSize = elementType->getReflector()->getSize();

			co::TypeKind ek = elementType->getKind();
			bool isPrimitive = ( ( ek >= co::TK_BOOLEAN && ek <= co::TK_DOUBLE ) || ek == co::TK_ENUM );
			co::uint32 flags = isPrimitive ? co::Any::VarIsValue : co::Any::VarIsConst|co::Any::VarIsReference;

			void* ptr = getArrayPtr( array ) + elementSize * index;

			element.setVariable( elementType, flags, ptr );
		}

		void escapeLuaString(const std::string&  str, stringstream& ss, bool surroundStringQuotes)
		{
			if(surroundStringQuotes)
			{
				ss << "'" << str << "'";
			}
			else
			{
				ss << str;
			}
		}

	};
	CORAL_EXPORT_COMPONENT(StringSerializer, StringSerializer);
};