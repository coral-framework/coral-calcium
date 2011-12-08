/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */
#ifndef _CA_SQLITERESULTSET_H_
#define _CA_SQLITERESULTSET_H_

#include <co/Coral.h>

extern "C"
{
	typedef struct sqlite3_stmt sqlite3_stmt;
}

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
