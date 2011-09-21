#include "SQLiteResultSet_Base.h"
#include "sqlite3.h"
#include <co/Coral.h>

#ifndef _CA_SQLITERESULTSET_H_
#define _CA_SQLITERESULTSET_H_

namespace ca
{
	class SQLiteResultSet : public SQLiteResultSet_Base
	{
	public:
		SQLiteResultSet(){}

		virtual ~SQLiteResultSet()
		{
			finalize();
		}

		bool next();
		
		const std::string& getValue( co::int32 columnIndex);

		void setStatement(sqlite3_stmt* stmt);

		void finalize();
		
	private:
		sqlite3_stmt* _stmt;
		std::string _value;
	};

};

#endif