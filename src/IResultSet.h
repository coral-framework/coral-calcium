#ifndef _CA_IRESULTSET_H_
#define _CA_IRESULTSET_H_

#include <co/Coral.h>

// ca.IResultSet Mapping:
namespace ca {

class IResultSet
{
public:
	virtual ~IResultSet() {;}

	virtual void finalize() = 0;

	virtual const std::string getValue( co::uint32 columnIndex ) = 0;

	virtual bool next() = 0;
};

} // namespace ca

#endif // _CA_IRESULTSET_H_
