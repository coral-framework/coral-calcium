/*******************************************************************************
** C++ mapping generated for type 'ca.MalformedSerializedStringException'
**
** Created: Mon Nov 07 14:13:46 2011
**      by: Coral Compiler version 0.7.0
**
** WARNING! All changes made in this file will be lost when recompiling!
********************************************************************************/

#ifndef _CA_MALFORMEDSERIALIZEDSTRINGEXCEPTION_H_
#define _CA_MALFORMEDSERIALIZEDSTRINGEXCEPTION_H_

#include <co/TypeTraits.h>
#include <co/Exception.h>

// ca.MalformedSerializedStringException Mapping:
namespace ca {

class MalformedSerializedStringException : public co::Exception
{
public:
	MalformedSerializedStringException()
	{;}

	MalformedSerializedStringException( const std::string& message )
		: co::Exception( message )
	{;}

	virtual ~MalformedSerializedStringException() throw()
	{;}

	inline const char* getTypeName() const { return "ca.MalformedSerializedStringException"; }
};

} // namespace ca

namespace co {
template<> struct kindOf<ca::MalformedSerializedStringException> : public kindOfBase<TK_EXCEPTION> {};
template<> struct nameOf<ca::MalformedSerializedStringException> { static const char* get() { return "ca.MalformedSerializedStringException"; } };
} // namespace co

#endif // _CA_MALFORMEDSERIALIZEDSTRINGEXCEPTION_H_
