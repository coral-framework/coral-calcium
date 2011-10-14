#include <co/Any.h>
#include <co/Coral.h>
#include <co/IType.h>
#include <cstdio>
#include <sstream>
#include <co/IClassType.h>
#include <co/IField.h>
#include <co/IReflector.h>
#include <ca/IModel.h>
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
			_model = 0;
		}

		void fromString( const std::string& valueToStr, co::IType* type, co::Any& value )
		{

			stringstream ss( valueToStr );

			fromStream( ss, type, value );

		}

		
		/**
			This method only applies for coral primitive types and arrays of primitive types, if the given string does not have type markup
		*/
		void fromString( const std::string& valueToStr, co::Any& value )
		{
			stringstream valueStream( valueToStr );

			readPrimitiveType( valueStream, value, co::TK_BOOLEAN);

			std::string rest; 
			valueStream >> rest;
		}

		void toString( const co::Any& value, std::string& valueToStr )
		{
			if( value.getKind() == co::TK_NONE )
			{
				throw co::IllegalArgumentException("Invalid value");
			}

			if( value.isPointer() || value.isPointerConst() )
			{
				throw co::IllegalArgumentException("Pointer serialization not supported");
			}
			
			stringstream valueStream;
			toStream(value, valueStream);
			valueToStr.assign(valueStream.str());
		}

	private:
		ca::IModel* _model;

		std::vector<co::IField*> getFieldsToSerializeForType( co::IClassType* type )
		{
			std::vector<co::IField*> fieldsToSerialize;
			if( _model == 0 )
			{
				for(int i = 0; i < type->getFields().getSize(); i++)
				{
					fieldsToSerialize.push_back( type->getFields()[i] );
				}
				
			}
			else
			{
				co::RefVector<co::IField> refVector;
				_model->getFields(type, refVector);
				for(int i = 0; i < type->getFields().getSize(); i++)
				{
					fieldsToSerialize.push_back( refVector[i].get() );
				}
			}
			return fieldsToSerialize;
		}

		void fromStream( stringstream& ss, co::IType* type, co::Any& value )
		{

			if( type->getKind() == co::TK_STRUCT || type->getKind() == co::TK_NATIVECLASS )
			{
				readComplexType( ss, value, type );
			}
			else if( type->getKind() == co::TK_ARRAY )
			{
				readArray(ss, value, type);
			}
			else if( type->getKind() == co::TK_ENUM )
			{
				co::int32 enumIntValue = readEnum(ss, static_cast<co::IEnum*>(type));
				value.set<co::int32>(enumIntValue);
			}
			else
			{
				readPrimitiveType(ss, value, type->getKind());
			}
		}

		void readComplexType( stringstream& ss, co::Any& value, co::IType* type )
		{
			
			co::IClassType* classType = static_cast<co::IClassType*>(type);

			char check;
			ss >> check;
			assert( check == '{' );

			std::vector<co::IField*> fields = getFieldsToSerializeForType(classType);
			std::string fieldName;
			co::Any fieldValue;

			co::IReflector* reflector = type->getReflector();
			value.createComplexValue(type);

			for( int i = 0; i < fields.size(); i++ )
			{
				fieldName = getLiteralFromStream(ss);

				assert( fieldName == fields[i]->getName() );

				ss >> check;
				assert( check == '=' );
				
				fromStream( ss, fields[i]->getType(), fieldValue );
				
				reflector->setField(value, fields[i], fieldValue);

				ss >> check;
				
			}
		}

		template< typename T>
		T readPrimitive(stringstream& ss, co::TypeKind tk)
		{
			int byte;
			std::string boolStr;
			T resultT;
			switch(tk)
			{
			case co::TK_BOOLEAN:
				boolStr = getLiteralFromStream( ss );
				assert(boolStr == "true" || boolStr == "false");
				resultT = (T)(boolStr == "true");
			break;
			case co::TK_INT8:
			case co::TK_UINT8:
				ss >> byte;
				resultT = (T)byte;
			break;
			case co::TK_INT16:
			case co::TK_UINT16:
			case co::TK_INT32:
			case co::TK_UINT32:
			case co::TK_INT64:
			case co::TK_UINT64:
			case co::TK_FLOAT:
			case co::TK_DOUBLE:
				ss >> resultT;
			break;

			}
			return resultT;
		}


		void readArray( std::stringstream &ss, co::Any& value, co::IType* type )
		{
			char brackets;

			ss >> brackets;

			assert( brackets == '{' );

			co::IArray* arrayType = (co::IArray*)type;
			co::IType* elementType = static_cast<co::IArray*>(type)->getElementType();
			co::TypeKind tk = elementType->getKind();

			co::Any indexArray;
			std::string literalTmp;
			co::Any::PseudoVector pseudoVector;
			switch(tk)
			{
				case co::TK_ARRAY:
					readArray( ss, indexArray, elementType);
				break;
				case co::TK_ENUM:
					getEnumArrayFromStream(ss, elementType, value);
				break;
				case co::TK_BOOLEAN:
					getPrimitiveArrayFromStream<bool>(ss, value, elementType);								
				break;
				case co::TK_INT8:
					getPrimitiveArrayFromStream<co::int8>(ss, value, elementType);
				break;
				case co::TK_UINT8:
					getPrimitiveArrayFromStream<co::uint8>(ss, value, elementType);
				break;
				case co::TK_INT16:
					getPrimitiveArrayFromStream<co::int16>(ss, value, elementType);
				break;
				case co::TK_UINT16:
					getPrimitiveArrayFromStream<co::uint16>(ss, value, elementType);
				break;
				case co::TK_INT32:
					getPrimitiveArrayFromStream<co::int32>(ss, value, elementType);
				break;
				case co::TK_UINT32:
					getPrimitiveArrayFromStream<co::uint32>(ss, value, elementType);
				break;
				case co::TK_INT64:
					getPrimitiveArrayFromStream<co::int64>(ss, value, elementType);
				break;
				case co::TK_UINT64:
					getPrimitiveArrayFromStream<co::uint64>(ss, value, elementType);
				break;
				case co::TK_FLOAT:
					getPrimitiveArrayFromStream<float>(ss, value, elementType);
				break;
				case co::TK_DOUBLE:
					getPrimitiveArrayFromStream<double>(ss, value, elementType);
				break;
				case co::TK_NATIVECLASS:
				case co::TK_STRUCT:
					getComplexTypeArrayFromStream( ss, elementType, value );
				break;
				case co::TK_STRING:
					getStringArrayFromStream(ss, value);
				break;
			}
		}

		void getComplexTypeArrayFromStream( stringstream& ss, co::IType* elementType, co::Any& value )
		{
			std::vector<co::Any> result;
			std::string literalTmp;
			char check;
			co::Any arrayElement;
			while(true)
			{
				readComplexType( ss, arrayElement, elementType );
				result.push_back(arrayElement);
				ss.get(check);
				if( check == '}' )
				{
					break;
				}
			}
			value.clear();
			value.createArray(elementType, result.size());
			for( int i = 0; i < result.size(); i++ )
			{
				setArrayComplexTypeElement(value, i, result[i]);
			}
		}

		void getStringArrayFromStream( std::stringstream& ss, co::Any& value)
		{
			vector<std::string> result;
			std::string literalTmp;
			char check;
			while(true)
			{
				literalTmp = removeQuotes(ss);
				result.push_back(literalTmp);
				ss.get(check);
				if( check == '}' )
				{
					break;
				}
			}
			value.clear();
			value.createArray(co::typeOf<std::string>::get(), result.size());
			
			for( int i = 0; i < result.size(); i++ )
			{
				setArrayElement<std::string>(value, i, result[i]);
			}
		}

		void getEnumArrayFromStream(stringstream& ss, co::IType* type, co::Any& value)
		{
			std::vector<co::int32> result;
			std::string literalTmp;
			co::IEnum* enumType = static_cast<co::IEnum*>(type);
			char check;
			while(true)
			{
				result.push_back(readEnum(ss, enumType));
				ss.get(check);
				if( check == '}' )
				{
					break;
				}
			}
			value.clear();
			value.createArray(type, result.size());
			for( int i = 0; i < result.size(); i++ )
			{
				setArrayElement<co::int32>(value, i, result[i]);
			}
		}

		co::int32 readEnum( stringstream& ss, co::IEnum* enumType )
		{
			std::string literalTmp = getLiteralFromStream(ss);
			return enumType->getValueOf(literalTmp);
		}

		std::string getLiteralFromStream(stringstream& ss)
		{
			std::string result;
			char buffer;

			ss.get(buffer);
			result.push_back(buffer);

			assert(isalpha(buffer));

			while(true)
			{
				buffer = ss.peek();
				
				if(ss.fail())
				{
					break;
				}
				
				if(isalnum(buffer))
				{
					result.push_back(buffer);
					ss.get( buffer );
				}
				else
				{
					break;
				}
			}
			return result;
		}

		template<typename T>
		void getPrimitiveArrayFromStream( std::stringstream& ss, co::Any& value, co::IType* elementType )
		{
			char check = ss.peek();
			std::vector<T> resultVector;
			if( check == '}' )
			{
				//emptyArray;
				value.createArray(elementType);
				ss.get(check);
				return;
			}

			T result;
			while(true)
			{
				result = readPrimitive<T>(ss, elementType->getKind());
				resultVector.push_back(result);
				ss.get(check);
				if( check == '}' )
				{
					break;
				}
				assert( check == ',' );
			}
			value.clear();
			value.createArray(elementType, resultVector.size());
			for( co::uint32 i = 0; i < resultVector.size(); i++)
			{
				setArrayElement<T>(value, i, resultVector[i]);
			}
		}

		template< typename T >
		void applyPrimitiveToAny( std::stringstream& ss, co::Any& value, co::TypeKind tk )
		{
			value.set<T>(readPrimitive<T>(ss, tk));
		}

		void readPrimitiveType( std::stringstream& ss, co::Any& value, co::TypeKind tk )
		{
			switch(tk)
			{
			case co::TK_BOOLEAN:
				applyPrimitiveToAny<bool>(ss, value, tk);
			break;
			case co::TK_INT8:
				applyPrimitiveToAny<co::int8>(ss, value, tk);
			break;
			case co::TK_UINT8:
				applyPrimitiveToAny<co::uint8>(ss, value, tk);
			break;
			case co::TK_INT16:
				applyPrimitiveToAny<co::int16>(ss, value, tk);
			break;
			case co::TK_UINT16:
				applyPrimitiveToAny<co::uint16>(ss, value, tk);
			break;
			case co::TK_INT32:
				applyPrimitiveToAny<co::int32>(ss, value, tk);
			break;
			case co::TK_UINT32:
				applyPrimitiveToAny<co::uint32>(ss, value, tk);
			break;
			case co::TK_INT64:
				applyPrimitiveToAny<co::int64>(ss, value, tk);
			break;
			case co::TK_UINT64:
				applyPrimitiveToAny<co::uint64>(ss, value, tk);
			break;
			case co::TK_FLOAT:
				applyPrimitiveToAny<float>(ss, value, tk);
			break;
			case co::TK_DOUBLE:
				applyPrimitiveToAny<double>(ss, value, tk);
			break;
			case co::TK_STRING:
				value.createString() = removeQuotes(ss);
			break;

			}
		}

		std::string removeQuotes( stringstream& ss )
		{
			if(ss.peek() == '\'')
			{
				return extractQuotedString(ss);
			}
			else
			{
				return extractLongBracketsString(ss);
			}
		}

		std::string extractQuotedString(stringstream& ss)
		{
			char readBuffer;
			std::string result;
			ss.get(readBuffer);

			assert(readBuffer == '\'');

			while(true)
			{
				ss.get(readBuffer);
				if( readBuffer == '\'' )
				{
					break;
				}
				result.push_back(readBuffer);
			}
			return result;
		}

		std::string extractLongBracketsString(stringstream& ss)
		{
			char readBuffer;
			char openBrackets[3];
			std::string result;
			ss.read(openBrackets, 3);

			int cmp = strcmp(openBrackets, "[=[");

			while(true)
			{
				ss.get(readBuffer);
				result.push_back(readBuffer);
				if( readBuffer == ']' )
				{
					std::string matchClose = result.substr(result.size() - 3, 3);

					if(matchClose == "]=]")
					{
						result = result.substr(0, result.size()-3);
						break;
					}
				}
			}
			return result;
		}

		void toStream( const co::Any& value, stringstream& ss )
		{
			co::IType* type = value.getType();

			if(type == NULL)
			{
				getBasicTypeString(value, ss);
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

			co::int8 enumAsint8 = value.get<co::int32>();
			
			ss << enumType->getIdentifiers()[enumAsint8];
		}

		void getComplexTypeString(const co::Any& value, stringstream& ss, co::IType* type)
		{
			co::IClassType * classType = static_cast<co::IClassType*>(type);
			std::vector<co::IField*> fields = getFieldsToSerializeForType( classType );
			co::IReflector * ref = type->getReflector();
			co::Any fieldValue;
			std::string fieldValueStr;
			ss << "{";
			for(int i = 0; i < fields.size(); i++)
			{
				co::IField* field = fields[i];

				ss << field->getName() << "=";

				ref->getField(value, field, fieldValue);
				
				toStream(fieldValue, ss);
				if( i != fields.size()-1 )
				{
					ss << ",";
				}
			}
			ss << "}";
		}

		void getArrayString(const co::Any& value, stringstream& ss, co::IType* type)
		{
			if( type->getKind() == co::TK_INTERFACE )
			{
				throw co::IllegalArgumentException("Pointer serialization not supported");
			}

			ss << "{";

			int size = getArraySize(value);
			co::Any arrayElement;

			for( int i = 0; i < size; i++ )
			{
				 getArrayElement(value, i, arrayElement);
				 toStream(arrayElement, ss);
				 
				 if( i < size-1)
				 {
					ss << ",";
				 }

			}
			ss << "}";

		}

		void getBasicTypeString(const co::Any& value, stringstream& ss)
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
				ss.precision( numeric_limits<long long int>::digits10 + 1 );
				ss << value.get<co::int64>();
			break;
			case co::TK_UINT64:
				ss.precision( numeric_limits<unsigned long long int>::digits10 + 1 );
				ss << value.get<co::uint64>();
			break;
			case co::TK_FLOAT:
				ss.precision( numeric_limits<float>::digits10 + 1 );
				ss << value.get<float>();
			break;
			case co::TK_DOUBLE:
				ss.precision( numeric_limits<double>::digits10 + 1 );
				ss << value.get<double>();
			break;
			case co::TK_STRING:
				std::string s = value.get<const std::string&>();
				escapeLuaString(s, ss );
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

		template< typename T>
		void setArrayElement( const co::Any& array, co::uint32 index, T element )
		{
			co::IType* elementType = array.getType();
			co::uint32 elementSize = elementType->getReflector()->getSize();

			co::uint8* p = getArrayPtr(array);

			co::TypeKind ek = elementType->getKind();
			bool isPrimitive = ( ( ek >= co::TK_BOOLEAN && ek <= co::TK_DOUBLE ) || ek == co::TK_ENUM );
			co::uint32 flags = isPrimitive ? co::Any::VarIsValue : co::Any::VarIsConst|co::Any::VarIsReference;

			void* ptr = getArrayPtr( array ) + elementSize * index;
			
			T* elementTypePointer = reinterpret_cast<T*>(ptr);
			*elementTypePointer = element;
		}

		void setArrayComplexTypeElement( const co::Any& array, co::uint32 index, co::Any& element )
		{
			co::IType* elementType = array.getType();
			co::IReflector* reflector = elementType->getReflector();
			co::uint32 elementSize = reflector->getSize();

			co::uint8* p = getArrayPtr(array);

			co::TypeKind ek = elementType->getKind();
			bool isPrimitive = ( ( ek >= co::TK_BOOLEAN && ek <= co::TK_DOUBLE ) || ek == co::TK_ENUM );
			co::uint32 flags = isPrimitive ? co::Any::VarIsValue : co::Any::VarIsConst|co::Any::VarIsReference;

			void* ptr = getArrayPtr( array ) + elementSize * index;
			
			reflector->copyValue( element.getState().data.ptr, ptr );

		}

		bool mustBeEscaped( const std::string& str )
		{
			size_t size = str.size();
			for( size_t i = 0; i < size; ++i )
			{
				char c = str[i];
				
				if( c == '\'' || iscntrl( c ) )
					return true;
			}
			return false;
		}

		void escapeLuaString(const std::string&  str, stringstream& ss)
		{
			if( !mustBeEscaped( str ) )
			{
				ss << "'" << str << "'";
			}
			else
			{
				ss << "[=[" << str << "]=]";
			}

		}

	};
	CORAL_EXPORT_COMPONENT(StringSerializer, StringSerializer);
};