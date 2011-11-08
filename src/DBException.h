/*******************************************************************************
** C++ mapping generated for type 'ca.DBException'
**
** Created: Mon Nov 07 13:31:15 2011
**      by: Coral Compiler version 0.7.0
**
** WARNING! All changes made in this file will be lost when recompiling!
********************************************************************************/

#ifndef _CA_DBEXCEPTION_H_
#define _CA_DBEXCEPTION_H_

#include <co/TypeTraits.h>
#include <co/Exception.h>

// ca.DBException Mapping:
namespace ca {

class DBException : public co::Exception
{
public:
	DBException()
	{;}

	DBException( const std::string& message )
		: co::Exception( message )
	{;}

	virtual ~DBException() throw()
	{;}

	inline const char* getTypeName() const { return "ca.DBException"; }
};

} // namespace ca

namespace co {
template<> struct kindOf<ca::DBException> : public kindOfBase<TK_EXCEPTION> {};
template<> struct nameOf<ca::DBException> { static const char* get() { return "ca.DBException"; } };
} // namespace co

#endif // _CA_DBEXCEPTION_H_
