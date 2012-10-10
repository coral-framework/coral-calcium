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

	void setModel( ca::IModel* model );

	void toString( const co::Any& value, std::string& result );

private:
	// serialization functions
	void streamOut( std::stringstream& ss, const co::Any& var );
	void writeString( std::stringstream& ss, const std::string& str );
	void writeRecord(std::stringstream& ss, const co::Any& var );
	void writeArray( std::stringstream& ss, const co::Any& array );

private:
	ca::IModel* _model;
};

} // namespace ca

#endif // _CA_ISTRINGSERIALIZER_H_
