#include "SQLiteDBConnection_Base.h"
#include "SQLiteResultSet.h"
#include "sqlite3.h"
#include <co/Coral.h>
#include <ca/DBException.h>
#include <ca/IResultSet.h>
#include <co/IObject.h>
#include <co/RefPtr.h>


namespace ca
{
	class SQLiteDBConnection : public SQLiteDBConnection_Base
	{
	public:
		SQLiteDBConnection()
		{
			_db = 0;
		}

		virtual ~SQLiteDBConnection()
		{
			if( _db ) 
			{
				close();
			}
		}

		void createDatabase()
		{
			if( fileExists( _fileName.c_str() ) )
			{
				throw ca::DBException("Create database failed. File already exists");
			}

			if( !sqlite3_open(_fileName.c_str(), &_db) == SQLITE_OK )
			{
				throw ca::DBException("Create database failed");
			}
		}

		void execute( const std::string& insertOrUpdateSQL ) 
		{
			if(!_db)
			{
				throw ca::DBException("Database not connected. Cannot execute command");
			}
			char* error;
			sqlite3_exec(_db, insertOrUpdateSQL.c_str(), 0, 0, &error);

			if(error)
			{
				throw ca::DBException(error);
			}
		}

		ca::IResultSet* executeQuery( const std::string& querySQL )
		{ 
			if(!_db)
			{
				throw ca::DBException( "Database not connected. Cannot execute command" );
			}

			int resultCode = sqlite3_prepare_v2( _db, querySQL.c_str(), -1, &statement, 0 );

			if(resultCode != SQLITE_OK)
			{
				char errorMsg[100];
				sprintf( errorMsg, "query failed: %s.", sqlite3_errmsg(_db) );
				throw ca::DBException(errorMsg);
			}

			co::IObject* resultSetObj = co::newInstance( "ca.SQLiteResultSet" );
			
			ca::SQLiteResultSet* resultSetSQLite = dynamic_cast<ca::SQLiteResultSet*>(resultSetObj);
						
			resultSetSQLite->setStatement(statement); 
			
			return resultSetObj->getService<ca::IResultSet>();
		}

		void open()
		{
			if(!fileExists(_fileName.c_str()))
			{
				throw ca::DBException( "Open database failed. Attempt to open non existing file" );
			}

			if( _db )
			{
				throw ca::DBException( "Open database failed. Database already opened" );
			}
			
			if(!sqlite3_open(_fileName.c_str(), &_db) == SQLITE_OK)
			{
				throw ca::DBException( "Open database failed" );
			}
		}

		void close()
		{
			int closeCode = sqlite3_close(_db);
			if(closeCode == SQLITE_OK)
			{
				_db = 0;
			}
			else {
				throw ca::DBException("Could not close database. Check for not finalized IResultSets");
			}
		}

		const std::string& getName()
		{
			return _fileName;
		}

		void setName( const std::string& name )
		{
			_fileName = name;
		}

	private:
		sqlite3* _db;
		std::string _fileName;
		sqlite3_stmt *statement;

		bool fileExists(const char * filePath)
		{
			FILE* file = fopen(filePath, "r");
			bool result = file != 0;
			if(result)
			{
				fclose(file);
			}
			return result;
		}
	};

	CORAL_EXPORT_COMPONENT(SQLiteDBConnection, SQLiteDBConnection);
};