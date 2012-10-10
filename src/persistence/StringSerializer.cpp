#include "StringSerializer.h"

#include <co/IField.h>
#include <co/IReflector.h>
#include <co/IllegalArgumentException.h>
#include <sstream>

namespace ca {

StringSerializer::StringSerializer()
{
	_model = NULL;
}

void StringSerializer::setModel( ca::IModel* model )
{
	_model = model;
}

void StringSerializer::toString( const co::Any& value, std::string& result )
{
	std::stringstream ss;
	streamOut( ss, value );
	ss.str().swap( result );
}

void StringSerializer::streamOut( std::stringstream& ss, const co::Any& var )
{
	co::IType* type = var.getType();
	co::TypeKind kind = type->getKind();
	if( co::isScalar( kind ) )
	{
		ss << var;
	}
	else if( kind == co::TK_STRING )
	{
		writeString( ss, var.get<const std::string&>() );
	}
	else if( co::isComplexValue( kind ) )
	{
		writeRecord( ss, var );
	}
	else if( kind == co::TK_ARRAY )
	{
		writeArray( ss, var.isIn() ? var : var.asIn() );
	}
	else
	{
		CORAL_THROW( co::IllegalArgumentException, "cannot serialize " << kind << " variables" );
	}
}

inline bool mustBeEscaped( const std::string& str )
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

void StringSerializer::writeString( std::stringstream& ss, const std::string& str )
{
	if( mustBeEscaped( str ) )
		ss << "[=[" << str << "]=]";
	else
		ss << "'" << str << "'";
}

void StringSerializer::writeRecord( std::stringstream& ss, const co::Any& var )
{
	assert( _model != NULL );

	co::RefVector<co::IField> fields;
	_model->getFields( static_cast<co::IRecordType*>( var.getType() ), fields );

	co::IReflector* reflector = var.getType()->getReflector();
	co::AnyValue value;

	ss << "{";
	for( size_t i = 0; i < fields.size(); ++i )
	{
		if( i > 0 )
			ss << ",";

		co::IField* field = fields[i].get();
		ss << field->getName() << "=";

		reflector->getField( var, field, value );
		streamOut( ss, value.getAny() );
	}
	ss << "}";
}

void StringSerializer::writeArray( std::stringstream& ss, const co::Any& array )
{
	ss << "{";
	size_t count = array.getCount();
	for( size_t i = 0; i < count; ++i )
	{
		if( i > 0 )
			ss << ",";
		streamOut( ss, array[i] );
	}
	ss << "}";
}

}; // namespace ca
