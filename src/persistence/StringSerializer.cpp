#include "StringSerializer.h"

#include <co/Any.h>
#include <co/Coral.h>
#include <co/IType.h>
#include <sstream>
#include <co/IClassType.h>
#include <co/IField.h>
#include <co/IReflector.h>
#include <ca/IModel.h>
#include <ca/ModelException.h>
#include <ca/FormatException.h>
#include <co/IllegalArgumentException.h>
#include <limits>

#include "AnyArrayUtil.h"

#ifdef CORAL_OS_WIN
#include <stdio.h>
#endif

namespace ca {

StringSerializer::StringSerializer()
{
	_model = NULL;
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

#ifdef CORAL_OS_WIN
	// makes windows compatible with the rest of the world
	_set_output_format( _TWO_DIGIT_EXPONENT );
#endif

	std::stringstream valueStream;
	toStream(value, valueStream);
	valueStream.str().swap( valueToStr );
}

void StringSerializer::setModel(ca::IModel* model)
{
	_model = model;
}


void StringSerializer::getFieldsToSerializeForType( co::IRecordType* type, std::vector<co::IField*>& fieldsToSerialize )
{
	if( _model == NULL )
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
		
		for(int i = 0; i < refVector.size(); i++)
		{
			fieldsToSerialize.push_back( refVector[i].get() );
		}
	}
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
		co::int32 enumIntValue = readEnum ( ss, co::cast<co::IEnum>( type ) );
		value.set<co::int32>( enumIntValue );
	}
	else
	{
		readPrimitiveType( ss, value, type );
	}
}

void StringSerializer::readComplexType( std::stringstream& ss, co::Any& value, co::IType* type )
{
			
	co::IRecordType* classType = co::cast<co::IRecordType>( type );

	char check;
	ss >> check;
	if( check != '{' )
	{
		throw ca::FormatException("'{' expected to start complex type value.");
	}

	value.createComplexValue( type );

	std::vector<co::IField*> fields; 
	getFieldsToSerializeForType( classType, fields );


	std::string fieldName;
	co::Any fieldValue;

	co::IReflector* reflector = type->getReflector();
	std::stringstream msg;

	for( int i = 0; i < fields.size(); i++ )
	{
		readLiteralFromStream( ss, fieldName );

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

void StringSerializer::readPrimitive( std::stringstream& ss, co::IType* type, void* result )
{
	co::TypeKind tk = type->getKind();

	co::uint32 typeSize = type->getReflector()->getSize();

	int byte;

	if( tk == co::TK_STRING )
	{
		extractStringValueWithoutQuotes( ss, *reinterpret_cast<std::string*>(result) );
	}

	switch( tk )
	{
	case co::TK_BOOLEAN:
		*reinterpret_cast<bool*>(result) = readBoolean(ss);
		break;
	case co::TK_INT8:

		ss >> byte;
		*reinterpret_cast<co::int8*>(result) = byte;
		break;
	case co::TK_UINT8:
		ss >> byte;
		*reinterpret_cast<co::uint8*>(result) = byte;
		break;
	case co::TK_INT16:
		ss >> *reinterpret_cast<co::int16*>(result);
		break;
	case co::TK_UINT16:
		ss >> *reinterpret_cast<co::uint16*>(result);
		break;		
	case co::TK_INT32:
		ss >> *reinterpret_cast<co::int32*>(result);
		break;		
	case co::TK_UINT32:
		ss >> *reinterpret_cast<co::uint32*>(result);
		break;		
	case co::TK_FLOAT:
		ss >> *reinterpret_cast<float*>(result);
		break;
	case co::TK_INT64:
		ss >> *reinterpret_cast<co::int64*>(result);
		break;
	case co::TK_UINT64:
		ss >> *reinterpret_cast<co::uint64*>(result);
		break;
	case co::TK_DOUBLE:
		ss >> *reinterpret_cast<double*>(result);
		break;
	default:
		assert( false );

	}
	assertNotFail( ss, " primitive type." );
}

bool StringSerializer::readBoolean( std::stringstream& ss )
{
	std::string boolStr;
	readLiteralFromStream( ss, boolStr );

	if (boolStr != "true" && boolStr != "false")
	{
		std::stringstream msg;
		msg << "Invalid boolean value: " << boolStr;
		throw ca::FormatException( msg.str() );
	}
	return (boolStr == "true");
}

void StringSerializer::setArrayElement( co::Any& value, co::uint32 index, void* element )
{
	AnyArrayUtil arrayUtil;
	if( value.getType()->getKind() == co::TK_STRUCT || value.getType()->getKind() == co::TK_NATIVECLASS )
	{
		arrayUtil.setArrayComplexTypeElement( value, index, *reinterpret_cast<co::Any*>( element ) );
	}
	else
	{
		arrayUtil.setArrayElement( value, index, element );
	}
}

void StringSerializer::readArray( std::stringstream &ss, co::Any& value, co::IType* type )
{
	char brackets;

	ss >> brackets;

	if( brackets != '{' )
	{
		throw ca::FormatException("'{' expected to start array value.");
	}
	co::IType* elementType = co::cast<co::IArray>( type )->getElementType();

	char check = ss.peek();

	std::vector<void*> result;

	if( check == '}' )
	{
		//emptyArray;
		value.createArray( elementType );
		ss.get(check);
		return;
	}

	co::TypeKind tk = elementType->getKind();

	co::Any* complexAny;
	while(true)
	{

		if( tk == co::TK_ENUM )
		{
			co::int32* enumInt = new co::int32;
			*enumInt = readEnum( ss, elementType );
			result.push_back( enumInt );
		}
		else if( tk == co::TK_STRUCT || tk == co::TK_NATIVECLASS )
		{
			complexAny = new co::Any;

			readComplexType( ss, *complexAny, elementType );
			result.push_back( complexAny );
		}
		else if( tk == co::TK_STRING )
		{
			std::string* stringValue = new std::string;

			extractStringValueWithoutQuotes( ss, *stringValue );
			result.push_back( stringValue );
		} 
		else
		{
			char* element = new char[elementType->getReflector()->getSize()];
			readPrimitive( ss, elementType, element );
			result.push_back( element );
		}


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
	value.createArray( elementType, result.size() );

	for( int i = 0; i < result.size(); i++ )
	{
		setArrayElement( value, i, result[i] );
		if( tk == co::TK_ENUM )
		{
			delete reinterpret_cast<co::int32*>(result[i]);
		}
		else if( tk == co::TK_STRUCT || tk == co::TK_NATIVECLASS )
		{
			delete reinterpret_cast<co::Any*>(result[i]);
		}
		else if( tk == co::TK_STRING )
		{
			delete reinterpret_cast<std::string*>(result[i]);
		} 
		else
		{
			delete [] reinterpret_cast<char*>(result[i]);
		}
	}

}

void StringSerializer::assertNotFail( std::stringstream& ss, std::string additionalInfo )
{
	if( ss.fail() )
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

co::int32 StringSerializer::readEnum( std::stringstream& ss, co::IType* type )
{
	std::string literalTmp; 
	readLiteralFromStream( ss, literalTmp );

	co::IEnum* enumType = co::cast<co::IEnum>( type );
	co::int32 enumValue = enumType->getValueOf( literalTmp );

	if( enumValue == -1 )
	{
		std::stringstream msg;
		msg << literalTmp << " is not a " << enumType->getFullName() << " valid literal.";
		throw ca::FormatException( msg.str() );
	}

	return enumValue;
}

void StringSerializer::readLiteralFromStream( std::stringstream& ss, std::string& str )
{
	std::string result;
	char buffer;

	str = "";

	ss.get( buffer );
	str.push_back( buffer );

	if( !isalpha( buffer ) )
	{
		throw ca::FormatException( "Literal expected, but control char found." );
	}

	while(true)
	{
		buffer = ss.peek();
				
		if( ss.fail() )
		{
			break;
		}
				
		if( isalnum( buffer ) )
		{
			str.push_back( buffer );
			ss.get( buffer );
		}
		else
		{
			break;
		}
	}
}

void StringSerializer::readPrimitiveType( std::stringstream& ss, co::Any& value, co::IType* type )
{
	if( type->getKind() == co::TK_STRING )
	{
		extractStringValueWithoutQuotes( ss, value.createString() );
	} 
	else
	{
		char* primitiveRead = new char[type->getReflector()->getSize()];
		readPrimitive( ss, type, primitiveRead );
		value.setBasic( type->getKind(), co::Any::VarIsValue || co::Any::VarIsReference, primitiveRead );
		delete[] primitiveRead;
	}
	
}

void StringSerializer::extractStringValueWithoutQuotes( std::stringstream& ss, std::string& str )
{
	if( ss.peek() == '\'')
	{
		return extractQuotedString( ss, str );
	}
	else
	{
		return extractLongBracketsString( ss, str );
	}
}

void StringSerializer::extractQuotedString( std::stringstream& ss, std::string& str )
{
	char readBuffer;
	ss.get(readBuffer);

	assert(readBuffer == '\'');
	str = "";
	while(true)
	{
		ss.get(readBuffer);

		assertNotFail( ss, "string value" );

		if( readBuffer == '\'' )
		{
			break;
		}
		str.push_back(readBuffer);
	}
}

void StringSerializer::extractLongBracketsString( std::stringstream& ss, std::string& str )
{
	char readBuffer;
	char openBrackets[4];

	ss.read(openBrackets, 3);
	openBrackets[3]='\0';
	
	assertNotFail( ss, "string value" );
	assert( std::string( openBrackets ) == "[=[" );

	str = "";
	while(true)
	{
		ss.get(readBuffer);

		assertNotFail( ss, "string value" );

		str.push_back(readBuffer);
		if( readBuffer == ']' )
		{
			std::string matchClose = str.substr(str.size() - 3, 3);

			if(matchClose == "]=]")
			{
				str = str.substr(0, str.size()-3);
				break;
			}
		}
	}
}

void StringSerializer::toStream( const co::Any& value, std::stringstream& ss )
{
	co::IType* type = value.getType();

	if( type == NULL )
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
	co::IEnum* enumType = co::cast<co::IEnum>( type );

	co::int8 enumAsint8 = value.get<co::int32>();
			
	ss << enumType->getIdentifiers()[enumAsint8];
}

void StringSerializer::writeComplexType( const co::Any& value, std::stringstream& ss, co::IType* type )
{
	co::IRecordType * recordType = co::cast<co::IRecordType>( type );
	std::vector<co::IField*> fields; 
	getFieldsToSerializeForType( recordType, fields );
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
			arrayUtil.getArrayElement( value, i, arrayElement );
			toStream( arrayElement, ss );
				 
			if( i < size-1)
			{
			ss << ",";
			}

	}
	ss << "}";

}

void StringSerializer::writeBasicType( const co::Any& value, std::stringstream& ss )
{
	co::TypeKind kind = value.getKind();
	if( kind == co::TK_BOOLEAN )
		ss << ( value.get<bool>() ? "true" : "false" );
	else if( kind == co::TK_STRING )
		escapeLuaString( value.get<const std::string&>(), ss );
	else
	{
		// 16 digits, sign, point, and \0
		char buffer[32];
		sprintf( buffer, "%.15g", value.get<double>() );
		ss << buffer;
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

void StringSerializer::escapeLuaString( const std::string& str, std::stringstream& ss )
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