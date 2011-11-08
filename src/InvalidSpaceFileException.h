/*******************************************************************************
** C++ mapping generated for type 'ca.InvalidSpaceFileException'
**
** Created: Mon Nov 07 14:13:46 2011
**      by: Coral Compiler version 0.7.0
**
** WARNING! All changes made in this file will be lost when recompiling!
********************************************************************************/

#ifndef _CA_INVALIDSPACEFILEEXCEPTION_H_
#define _CA_INVALIDSPACEFILEEXCEPTION_H_

#include <co/TypeTraits.h>
#include <co/Exception.h>

// ca.InvalidSpaceFileException Mapping:
namespace ca {

class InvalidSpaceFileException : public co::Exception
{
public:
	InvalidSpaceFileException()
	{;}

	InvalidSpaceFileException( const std::string& message )
		: co::Exception( message )
	{;}

	virtual ~InvalidSpaceFileException() throw()
	{;}

	inline const char* getTypeName() const { return "ca.InvalidSpaceFileException"; }
};

} // namespace ca

namespace co {
template<> struct kindOf<ca::InvalidSpaceFileException> : public kindOfBase<TK_EXCEPTION> {};
template<> struct nameOf<ca::InvalidSpaceFileException> { static const char* get() { return "ca.InvalidSpaceFileException"; } };
} // namespace co

#endif // _CA_INVALIDSPACEFILEEXCEPTION_H_
