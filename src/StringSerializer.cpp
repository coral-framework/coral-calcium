#include "StringSerializer.h"

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
#include <ca/FormatException.h>
#include <co/IllegalArgumentException.h>
#include <limits>

namespace ca {

StringSerializer::StringSerializer()
{
	_model = 0;
}

void StringSerializer::fromString( const std::string& valueToStr, co::IType* type, co::Any& value )
{

	std::stringstream ss( valueToStr );

	fromStream( ss, type, value );

}

void StringSerializer::toString( const co::Any& value, std::string& valueToStr )
{
	if( value.getKind() == co::TK_NONE )
	{
		throw co::IllegalArgumentException("Invalid value");
	}

	if( value.isPointer() || value.isPointerConst() )
	{
		throw co::IllegalArgumentException("Pointer serialization not supported");
	}
			
	std::stringstream valueStream;
	toStream(value, valueStream);
	valueToStr.assign(valueStream.str());
}

void StringSerializer::setModel(ca::IModel* model)
{
	_model = model;
}


std::vector<co::IField*> StringSerializer::getFieldsToSerializeForType( co::IRecordType* type )
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
		catch ( ca::ModelException& e )
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

void StringSerializer::fromStream( std::stringstream& ss, co::IType* type, co::Any& value )
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

void StringSerializer::readComplexType( std::stringstream& ss, co::Any& value, co::IType* type )
{
			
	co::IClassType* classType = static_cast<co::IClassType*>(type);

	char check;
	ss >> check;
	if( check != '{' )
	{
		throw ca::FormatException("'{' expected to start complex type value.");
	}

	std::vector<co::IField*> fields = getFieldsToSerializeForType(classType);
	std::string fieldName;
	co::Any fieldValue;

	co::IReflector* reflector = type->getReflector();
	value.createComplexValue(type);
	std::stringstream msg;
	for( int i = 0; i < fields.size(); i++ )
	{
		fieldName = readLiteralFromStream(ss);

		if( fieldName != fields[i]->getName() )
		{
			msg.clear();
			msg << "Invalid field name '" << fieldName << "' for type " << type->getFullName();
			throw ca::FormatException( msg.str() );
		}

		ss >> check;
				
		if( check != '=' )
		{
			throw ca::FormatException(" '=' expected after field name");
		}
				
		fromStream( ss, fields[i]->getType(), fieldValue );
				
		try
		{
			reflector->setField(value, fields[i], fieldValue);
		}
		catch( co::IllegalArgumentException& e )
		{
			msg.clear();
			msg << "Could not desserialize type " << type->getFullName() << " " << e.getMessage();
			throw ca::FormatException( msg.str() );
		}
				
		ss >> check;
		assertNotFail( ss, type->getFullName() );
				
		if( ( ( i == fields.size() - 1 ) && (check != '}') ) )
		{
			msg.clear();
			msg << "'}' expected to end type " << type->getFullName();
			throw ca::FormatException( msg.str() );
		}

		if( ( ( i >=0 ) && ( i < fields.size() - 1 ) && (check != ',') ) )
		{
			msg.clear();
			msg << "',' expected to separate fields of type " << type->getFullName();
			throw ca::FormatException( msg.str() );
		}

	}
}

template< typename T >
T StringSerializer::readPrimitive( std::stringstream& ss, co::TypeKind tk )
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

bool StringSerializer::readBoolean( std::stringstream& ss )
{
	std::string boolStr;
	boolStr = readLiteralFromStream( ss );

	if (boolStr != "true" && boolStr != "false")
	{
		std::stringstream msg;
		msg << "Invalid boolean value: " << boolStr;
		throw ca::FormatException( msg.str() );
	}
	return (boolStr == "true");
}

void StringSerializer::readArray( std::stringstream &ss, co::Any& value, co::IType* type )
{
	char brackets;

	ss >> brackets;

	if( brackets != '{' )
	{
		throw ca::FormatException("'{' expected to start array value.");
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
			readEnumArrayFromStream(ss, elementType, value);
		break;
		case co::TK_BOOLEAN:
			readPrimitiveArrayFromStream<bool>(ss, value, elementType);								
		break;
		case co::TK_INT8:
			readPrimitiveArrayFromStream<co::int8>(ss, value, elementType);
		break;
		case co::TK_UINT8:
			readPrimitiveArrayFromStream<co::uint8>(ss, value, elementType);
		break;
		case co::TK_INT16:
			readPrimitiveArrayFromStream<co::int16>(ss, value, elementType);
		break;
		case co::TK_UINT16:
			readPrimitiveArrayFromStream<co::uint16>(ss, value, elementType);
		break;
		case co::TK_INT32:
			readPrimitiveArrayFromStream<co::int32>(ss, value, elementType);
		break;
		case co::TK_UINT32:
			readPrimitiveArrayFromStream<co::uint32>(ss, value, elementType);
		break;
		case co::TK_INT64:
			readPrimitiveArrayFromStream<co::int64>(ss, value, elementType);
		break;
		case co::TK_UINT64:
			readPrimitiveArrayFromStream<co::uint64>(ss, value, elementType);
		break;
		case co::TK_FLOAT:
			readPrimitiveArrayFromStream<float>(ss, value, elementType);
		break;
		case co::TK_DOUBLE:
			readPrimitiveArrayFromStream<double>(ss, value, elementType);
		break;
		case co::TK_NATIVECLASS:
		case co::TK_STRUCT:
			readComplexTypeArrayFromStream( ss, elementType, value );
		break;
		case co::TK_STRING:
			readStringArrayFromStream(ss, value);
		break;
	}
}

void StringSerializer::readComplexTypeArrayFromStream( std::stringstream& ss, co::IType* elementType, co::Any& value )
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

void StringSerializer::assertNotFail( std::stringstream& ss, std::string additionalInfo )
{
	if(ss.fail())
	{
		std::stringstream msg;
		msg << "Unexpected end of string on " << additionalInfo << ".";
		throw ca::FormatException(msg.str());
	}
}

void StringSerializer::assertNotInvalidArrayChar( char check )
{
	if( check != ',' )
	{
		throw ca::FormatException("Array format not valid, ',' or '}' expected.");
	} 
}

void StringSerializer::readStringArrayFromStream( std::stringstream& ss, co::Any& value)
{
	std::vector<std::string> result;
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

void StringSerializer::readEnumArrayFromStream( std::stringstream& ss, co::IType* type, co::Any& value )
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
			throw ca::FormatException("Array format not valid, ',' or '}' expected.");
		}
	}
	value.clear();
	value.createArray(type, result.size());
	for( int i = 0; i < result.size(); i++ )
	{
		arrayUtil.setArrayElement<co::int32>(value, i, result[i]);
	}
}

co::int32 StringSerializer::readEnum( std::stringstream& ss, co::IEnum* enumType )
{
	std::string literalTmp = readLiteralFromStream(ss);
	co::int32 enumValue = enumType->getValueOf(literalTmp);

	if( enumValue == -1 )
	{
		std::stringstream msg;
		msg << literalTmp << " is not a " << enumType->getFullName() << " valid literal.";
		throw ca::FormatException( msg.str() );
	}

	return enumValue;
}

std::string StringSerializer::readLiteralFromStream( std::stringstream& ss )
{
	std::string result;
	char buffer;

	ss.get(buffer);
	result.push_back(buffer);

	if(!isalpha(buffer))
	{
		throw ca::FormatException( "Literal expected, but control char found." );
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
void StringSerializer::readPrimitiveArrayFromStream( std::stringstream& ss, co::Any& value, co::IType* elementType )
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
	std::stringstream errorMsg;
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
void StringSerializer::applyPrimitiveToAny( std::stringstream& ss, co::Any& value, co::TypeKind tk )
{
	value.set<T>(readPrimitive<T>(ss, tk));
}

void StringSerializer::readPrimitiveType( std::stringstream& ss, co::Any& value, co::TypeKind tk )
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

std::string StringSerializer::extractStringValueWithoutQuotes( std::stringstream& ss )
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

std::string StringSerializer::extractQuotedString( std::stringstream& ss )
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

std::string StringSerializer::extractLongBracketsString( std::stringstream& ss )
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

void StringSerializer::toStream( const co::Any& value, std::stringstream& ss )
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

void StringSerializer::writeEnum( const co::Any& value, std::stringstream& ss, co::IType* type )
{
	co::IEnum* enumType = static_cast<co::IEnum*>(type);

	co::int8 enumAsint8 = value.get<co::int32>();
			
	ss << enumType->getIdentifiers()[enumAsint8];
}

void StringSerializer::writeComplexType( const co::Any& value, std::stringstream& ss, co::IType* type )
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

void StringSerializer::writeArray(const co::Any& value, std::stringstream& ss, co::IType* type)
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

void StringSerializer::writeBasicType(const co::Any& value, std::stringstream& ss)
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
		ss.precision( std::numeric_limits<long int>::digits10 + 1 );
		ss << value.get<co::int32>();
	break;
	case co::TK_UINT32:
		ss.precision( std::numeric_limits<unsigned long int>::digits10 + 1);
		ss << value.get<co::uint32>();
	break;
	case co::TK_INT64:
		ss.precision( std::numeric_limits<long long int>::digits10 + 1 );
		ss << value.get<co::int64>();
	break;
	case co::TK_UINT64:
		ss.precision( std::numeric_limits<unsigned long long int>::digits10 + 1 );
		ss << value.get<co::uint64>();
	break;
	case co::TK_FLOAT:
		ss.precision( std::numeric_limits<float>::digits10 + 1 );
		ss << value.get<float>();
	break;
	case co::TK_DOUBLE:
		ss.precision( std::numeric_limits<double>::digits10 + 1 );
		ss << value.get<double>();
	break;
	case co::TK_STRING:
		std::string s = value.get<const std::string&>();
		escapeLuaString(s, ss );
	break;

	}
}

bool StringSerializer::mustBeEscaped( const std::string& str )
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

void StringSerializer::escapeLuaString( const std::string&  str, std::stringstream& ss )
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

}; // namespace ca