#include "StringSerializer.h"

#include <co/Coral.h>
#include <co/IField.h>
#include <sstream>
#include <co/IllegalArgumentException.h>

#include "AnyArrayUtil.h"

#ifdef CORAL_OS_WIN
#include <stdio.h>
#endif

namespace ca {

StringSerializer::StringSerializer()
{
	_model = NULL;
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
	co::IRecordType * recordType = static_cast<co::IRecordType*>( type );
	std::vector<co::IField*> fields;
	getFieldsToSerializeForType( recordType, fields );
	co::IReflector * ref = type->getReflector();
	co::Any fieldValue;

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
				 
			if( i < size-1 )
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
	{
		ss << ( value.get<bool>() ? "true" : "false" );
	}
	else if( kind == co::TK_STRING )
	{
		escapeLuaString( value.get<const std::string&>(), ss );
	}
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