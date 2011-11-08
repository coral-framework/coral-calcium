#include "sqlite3.h"
#include "IResultSet.h"
#include <co/Coral.h>

#ifndef _CA_SQLITERESULTSET_H_
#define _CA_SQLITERESULTSET_H_

namespace ca
{
	class SQLiteResultSet : public IResultSet
	{
	public:
		SQLiteResultSet(){}

		virtual ~SQLiteResultSet()
		{
			finalize();
		}

		virtual bool next();
		
		virtual const std::string& getValue( co::int32 columnIndex);

		virtual void finalize();

		virtual void setStatement(sqlite3_stmt* stmt);
		
	private:
		sqlite3_stmt* _stmt;
		std::string _value;
	};

}

#endif