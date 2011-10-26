#include <co/Any.h>
#include <co/Coral.h>
#include <co/IType.h>
#include <cstdio>
#include <sstream>
#include <co/IClassType.h>
#include <co/IField.h>
#include <co/IReflector.h>
#include <ca/IModel.h>
#include <ca/ModelException.h>
#include <co/IllegalArgumentException.h>
#include <ca/MalformedSerializedStringException.h>
#include <limits>
using namespace std;

#include "StringSerializer_Base.h"
#include "AnyArrayUtil.h"

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

		ca::IModel* getModelService()
		{
			return _model;
		}

		void setModelService(ca::IModel* model)
		{
			_model = model;
		}

	private:
		ca::IModel* _model;

		std::vector<co::IField*> getFieldsToSerializeForType( co::IRecordType* type )
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
				
				try 
				{
					_model->getFields(type, refVector);
				}
				catch ( ca::ModelException e )
				{
					std::string msg = e.getMessage();
					printf( msg.c_str() );
				}

				for(int i = 0; i < refVector.size(); i++)
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
			if( check != '{' )
			{
				throw ca::MalformedSerializedStringException("'{' expected to start complex type value.");
			}

			std::vector<co::IField*> fields = getFieldsToSerializeForType(classType);
			std::string fieldName;
			co::Any fieldValue;

			co::IReflector* reflector = type->getReflector();
			value.createComplexValue(type);
			stringstream msg;
			for( int i = 0; i < fields.size(); i++ )
			{
				fieldName = getLiteralFromStream(ss);

				if( fieldName != fields[i]->getName() )
				{
					msg.clear();
					msg << "Invalid field name '" << fieldName << "' for type " << type->getFullName();
					throw ca::MalformedSerializedStringException( msg.str() );
				}

				ss >> check;
				
				if( check != '=' )
				{
					throw ca::MalformedSerializedStringException(" '=' expected after field name");
				}
				
				fromStream( ss, fields[i]->getType(), fieldValue );
				
				try
				{
					reflector->setField(value, fields[i], fieldValue);
				}
				catch( co::IllegalArgumentException e)
				{
					msg.clear();
					msg << "Could not desserialize type " << type->getFullName() << " " << e.getMessage();
					throw ca::MalformedSerializedStringException( msg.str() );
				}
				
				ss >> check;
				assertNotFail( ss, type->getFullName() );
				
				if( ( ( i == fields.size() - 1 ) && (check != '}') ) )
				{
					msg.clear();
					msg << "'}' expected to end type " << type->getFullName();
					throw ca::MalformedSerializedStringException( msg.str() );
				}

				if( ( ( i >=0 ) && ( i < fields.size() - 1 ) && (check != ',') ) )
				{
					msg.clear();
					msg << "',' expected to separate fields of type " << type->getFullName();
					throw ca::MalformedSerializedStringException( msg.str() );
				}

			}
		}

		template< typename T>
		T readPrimitive(stringstream& ss, co::TypeKind tk)
		{
			int byte;
			T resultT;
			switch(tk)
			{
			case co::TK_BOOLEAN:
				return (T)readBoolean(ss);
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
			assertNotFail( ss, " primitive type." );
			return resultT;
		}

		bool readBoolean( stringstream& ss )
		{
			std::string boolStr;
			boolStr = getLiteralFromStream( ss );

			if (boolStr != "true" && boolStr != "false")
			{
				stringstream msg;
				msg << "Invalid boolean value: " << boolStr;
				throw ca::MalformedSerializedStringException( msg.str() );
			}
			return (boolStr == "true");
		}

		void readArray( std::stringstream &ss, co::Any& value, co::IType* type )
		{
			char brackets;

			ss >> brackets;

			if( brackets != '{' )
			{
				throw ca::MalformedSerializedStringException("'{' expected to start array value.");
			}

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
			AnyArrayUtil arrayUtil;
			std::string literalTmp;
			char check;
			co::Any arrayElement;
			while(true)
			{
				readComplexType( ss, arrayElement, elementType );
				result.push_back(arrayElement);
				ss.get(check);
				assertNotFail( ss, elementType->getFullName() );
				if( check == '}' )
				{
					break;
				}

				assertNotInvalidArrayChar( check );

			}
			value.clear();
			value.createArray(elementType, result.size());
			for( int i = 0; i < result.size(); i++ )
			{
				arrayUtil.setArrayComplexTypeElement(value, i, result[i]);
			}
		}

		void assertNotFail( stringstream& ss, std::string additionalInfo )
		{
			if(ss.fail())
			{
				stringstream msg;
				msg << "Unexpected end of string on " << additionalInfo << ".";
				throw ca::MalformedSerializedStringException(msg.str());
			}
		}

		void assertNotInvalidArrayChar( char check )
		{
			if( check != ',' )
			{
				throw ca::MalformedSerializedStringException("Array format not valid, ',' or '}' expected.");
			} 
		}

		void getStringArrayFromStream( std::stringstream& ss, co::Any& value)
		{
			vector<std::string> result;
			std::string literalTmp;

			AnyArrayUtil arrayUtil;

			char check;
			while(true)
			{
				literalTmp = extractStringValueWithoutQuotes(ss);
				result.push_back(literalTmp);
				ss.get(check);
				assertNotFail(ss, "string array");
				if( check == '}' )
				{
					break;
				}
				assertNotInvalidArrayChar( check );
			}
			value.clear();
			value.createArray(co::typeOf<std::string>::get(), result.size());
			
			for( int i = 0; i < result.size(); i++ )
			{
				arrayUtil.setArrayElement<std::string>(value, i, result[i]);
			}
		}

		void getEnumArrayFromStream(stringstream& ss, co::IType* type, co::Any& value)
		{
			std::vector<co::int32> result;
			std::string literalTmp;

			AnyArrayUtil arrayUtil;

			co::IEnum* enumType = static_cast<co::IEnum*>(type);
			char check;
			while(true)
			{
				result.push_back(readEnum(ss, enumType));
				ss.get(check);
				assertNotFail( ss, "enum array" );

				if( check == '}' )
				{
					break;
				}
				if( check != ',' )
				{
					throw ca::MalformedSerializedStringException("Array format not valid, ',' or '}' expected.");
				}
			}
			value.clear();
			value.createArray(type, result.size());
			for( int i = 0; i < result.size(); i++ )
			{
				arrayUtil.setArrayElement<co::int32>(value, i, result[i]);
			}
		}

		co::int32 readEnum( stringstream& ss, co::IEnum* enumType )
		{
			std::string literalTmp = getLiteralFromStream(ss);
			co::int32 enumValue = enumType->getValueOf(literalTmp);

			if( enumValue == -1 )
			{
				stringstream msg;
				msg << literalTmp << " is not a " << enumType->getFullName() << " valid literal.";
				throw ca::MalformedSerializedStringException( msg.str() );
			}

			return enumValue;
		}

		std::string getLiteralFromStream(stringstream& ss)
		{
			std::string result;
			char buffer;

			ss.get(buffer);
			result.push_back(buffer);

			if(!isalpha(buffer))
			{
				throw ca::MalformedSerializedStringException( "Literal expected, but control char found." );
			}

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

			AnyArrayUtil arrayUtil;

			if( check == '}' )
			{
				//emptyArray;
				value.createArray(elementType);
				ss.get(check);
				return;
			}

			T result;
			stringstream errorMsg;
			errorMsg << elementType->getFullName() << " array";
			while(true)
			{
				result = readPrimitive<T>(ss, elementType->getKind());
				resultVector.push_back(result);
				ss.get(check);
				assertNotFail( ss, errorMsg.str() );
				if( check == '}' )
				{
					break;
				}
				assertNotInvalidArrayChar( check );
			}
			value.clear();
			value.createArray(elementType, resultVector.size());
			for( co::uint32 i = 0; i < resultVector.size(); i++)
			{
				arrayUtil.setArrayElement<T>(value, i, resultVector[i]);
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
				value.createString() = extractStringValueWithoutQuotes(ss);
			break;

			}
		}

		std::string extractStringValueWithoutQuotes( stringstream& ss )
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

				assertNotFail( ss, "string value" );

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
			char openBrackets[4];
			std::string result;
			ss.read(openBrackets, 3);

			int cmp = strcmp(openBrackets, "[=[");

			while(true)
			{
				ss.get(readBuffer);

				assertNotFail( ss, "string value" );

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
				writeBasicType(value, ss);
			}
			else
			{
				co::TypeKind kind = value.getKind();
				if( kind == co::TK_STRUCT || kind == co::TK_NATIVECLASS )
				{
					writeComplexType( value, ss, type );
				}
				if( kind == co::TK_ARRAY )
				{
					writeArray( value, ss, type );
				}
				if( kind == co::TK_ENUM )
				{
					writeEnum( value, ss, type );
				}
			}
		}

		void writeEnum(const co::Any& value, stringstream& ss, co::IType* type)
		{
			co::IEnum* enumType = static_cast<co::IEnum*>(type);

			co::int8 enumAsint8 = value.get<co::int32>();
			
			ss << enumType->getIdentifiers()[enumAsint8];
		}

		void writeComplexType(const co::Any& value, stringstream& ss, co::IType* type)
		{
			co::IRecordType * recordType = static_cast<co::IRecordType*>(type);
			std::vector<co::IField*> fields = getFieldsToSerializeForType( recordType );
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

		void writeArray(const co::Any& value, stringstream& ss, co::IType* type)
		{
			AnyArrayUtil arrayUtil;
			if( type->getKind() == co::TK_INTERFACE )
			{
				throw co::IllegalArgumentException("Pointer serialization not supported");
			}

			ss << "{";

			int size = arrayUtil.getArraySize(value);
			co::Any arrayElement;

			for( int i = 0; i < size; i++ )
			{
				 arrayUtil.getArrayElement(value, i, arrayElement);
				 toStream(arrayElement, ss);
				 
				 if( i < size-1)
				 {
					ss << ",";
				 }

			}
			ss << "}";

		}

		void writeBasicType(const co::Any& value, stringstream& ss)
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