#include "sqlite3.h"
#include <co/Coral.h>

#ifndef _CA_SQLITERESULTSET_H_
#define _CA_SQLITERESULTSET_H_

namespace ca {

class SQLiteResultSet
{
public:
	SQLiteResultSet()
	{
		_stmt = NULL;
	}

	virtual ~SQLiteResultSet()
	{
		finalize();
	}

	bool next();
		
	const std::string getValue( co::uint32 columnIndex);

	void finalize();

	void setStatement( sqlite3_stmt* stmt, bool prepared );
	
private:
	sqlite3_stmt* _stmt;
	std::string _value;
	unsigned int _columnCount;
	bool _isPreparedStatement;
};

} // namespace ca

#endif