/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_STRINGSERIALIZER_H_
#define _CA_STRINGSERIALIZER_H_

#include <co/Any.h>
#include <sstream>
#include <string>
#include <ca/IModel.h>

namespace ca {

class StringSerializer
{
public:
	StringSerializer();

	void fromString( const std::string& valueToStr, co::IType* type, co::Any& value );

	void toString( const co::Any& value, std::string& valueToStr );

	void setModel( ca::IModel* model );

private:
	//determine which fields should be serialized. If a IModel is provided, it is used to tell the fields to be serialized.
	//If not, all fields of the given type will be serialized.
	void getFieldsToSerializeForType( co::IRecordType* type, std::vector<co::IField*>& fields );

	//de-serialization functions

	void fromStream( std::stringstream& ss, co::IType* type, co::Any& value );

	void readComplexType( std::stringstream& ss, co::Any& value, co::IType* type );

	void readPrimitive( std::stringstream& ss, co::IType* type, void* result );

	bool readBoolean( std::stringstream& ss );

	void readArray( std::stringstream &ss, co::Any& value, co::IType* type );

	co::int32 readEnum( std::stringstream& ss, co::IType* type );

	void readLiteralFromStream( std::stringstream& ss, std::string& str );

	void readPrimitiveType( std::stringstream& ss, co::Any& value, co::IType* type );

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
	void extractStringValueWithoutQuotes( std::stringstream& ss, std::string& str );

	void extractQuotedString( std::stringstream& ss, std::string& str );

	void extractLongBracketsString( std::stringstream& ss, std::string& str  );

	bool mustBeEscaped( const std::string& str );

	void escapeLuaString( const std::string& str, std::stringstream& ss );

	void setArrayElement( co::Any& value, co::uint32 index, void* element );
private:
	ca::IModel* _model;

};

} // namespace ca

#endif // _CA_ISTRINGSERIALIZER_H_
