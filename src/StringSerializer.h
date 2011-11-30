#ifndef _CA_STRINGSERIALIZER_H_
#define _CA_STRINGSERIALIZER_H_

#include <co/Any.h>
#include <sstream>
#include <string>
#include <ca/IModel.h>
#include "AnyArrayUtil.h"

namespace ca {

class StringSerializer
{
	
public:

	StringSerializer();

	~StringSerializer() {;}

	void fromString( const std::string& valueToStr, co::IType* type, co::Any& value );

	void toString( const co::Any& value, std::string& valueToStr );

	void setModel( ca::IModel* model );

private:

	ca::IModel* _model;

	//determine which fields should be serialized. If a IModel is provided, it is used to tell the fields to be serialized.
	//If not, all fields of the given type will be serialized.
	std::vector<co::IField*> getFieldsToSerializeForType( co::IRecordType* type );

	//de-serialization functions

	void fromStream( std::stringstream& ss, co::IType* type, co::Any& value );

	void readComplexType( std::stringstream& ss, co::Any& value, co::IType* type );

	template< typename T>
	T readPrimitive( std::stringstream& ss, co::TypeKind tk );

	bool readBoolean( std::stringstream& ss );

	void readArray( std::stringstream &ss, co::Any& value, co::IType* type );

	void readComplexTypeArrayFromStream( std::stringstream& ss, co::IType* elementType, co::Any& value );

	void readStringArrayFromStream( std::stringstream& ss, co::Any& value);

	void readEnumArrayFromStream( std::stringstream& ss, co::IType* type, co::Any& value);

	co::int32 readEnum( std::stringstream& ss, co::IEnum* enumType );

	std::string readLiteralFromStream( std::stringstream& ss );
	
	template<typename T>
	void readPrimitiveArrayFromStream( std::stringstream& ss, co::Any& value, co::IType* elementType );

	void readPrimitiveType( std::stringstream& ss, co::Any& value, co::TypeKind tk );

	//any helper function
	template< typename T >
	void applyPrimitiveToAny( std::stringstream& ss, co::Any& value, co::TypeKind tk );

	//stream read error check functions
	void assertNotFail( std::stringstream& ss, std::string additionalInfo );

	void assertNotInvalidArrayChar( char check );
	
	//serialization functions
	void toStream( const co::Any& value, std::stringstream& ss );

	void writeEnum(const co::Any& value, std::stringstream& ss, co::IType* type);

	void writeArray(const co::Any& value, std::stringstream& ss, co::IType* type);

	void writeBasicType(const co::Any& value, std::stringstream& ss);

	void writeComplexType(const co::Any& value, std::stringstream& ss, co::IType* type);

	//string values helpers
	std::string extractStringValueWithoutQuotes( std::stringstream& ss );

	std::string extractQuotedString( std::stringstream& ss );

	std::string extractLongBracketsString( std::stringstream& ss );

	bool mustBeEscaped( const std::string& str );

	void escapeLuaString( const std::string&  str, std::stringstream& ss );

};

} // namespace ca
#endif // _CA_ISTRINGSERIALIZER_H_
