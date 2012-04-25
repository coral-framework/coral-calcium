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

	void toString( const co::Any& value, std::string& valueToStr );

	void setModel( ca::IModel* model );
private:
	//determine which fields should be serialized. If a IModel is provided, it is used to tell the fields to be serialized.
	//If not, all fields of the given type will be serialized.
	void getFieldsToSerializeForType( co::IRecordType* type, std::vector<co::IField*>& fields );

	//serialization functions
	void toStream( const co::Any& value, std::stringstream& ss );

	void writeEnum(const co::Any& value, std::stringstream& ss, co::IType* type);

	void writeArray(const co::Any& value, std::stringstream& ss, co::IType* type);

	void writeBasicType(const co::Any& value, std::stringstream& ss);

	void writeComplexType(const co::Any& value, std::stringstream& ss, co::IType* type);

	//string values helpers
	bool mustBeEscaped( const std::string& str );

	void escapeLuaString( const std::string& str, std::stringstream& ss );
private:
	ca::IModel* _model;
};

} // namespace ca

#endif // _CA_ISTRINGSERIALIZER_H_
