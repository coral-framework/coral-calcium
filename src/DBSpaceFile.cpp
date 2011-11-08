#include "DBSpaceFile_Base.h"
#include "SpaceSaverSQLQueries.h"

#include <co/RefPtr.h>
#include <co/Range.h>

#include <ca/InvalidSpaceFileException.h>


#include <ca/FieldValueBean.h>
#include <ca/EntityBean.h>

#include <ca/DBException.h>

#include "SQLiteDBConnection.h"
#include "IResultSet.h"
#include "SQLiteResultSet.h"

#include <string>
using namespace std;

namespace ca {

	class DBSpaceFile : public DBSpaceFile_Base
	{
	public:
		DBSpaceFile()
		{
		}

		virtual ~DBSpaceFile()
		{
			finalize();
		}

		void initialize()
		{
			
			assert( !_fileName.empty() );
			_db.setFileName( _fileName );
			
			try
			{
				_db.createDatabase();
			}
			catch( ca::DBException e )
			{
				try
				{
					_db.open();
				}
				catch( ca::DBException e )
				{
					throw e;
				}
			}
			createTables();
			insertModel();
		}

		void finalize()
		{
			try
			{
				_db.close();
			}
			catch( ca::DBException e )
			{
				throw e;
			}
		}

		co::int32 insertEntity( const string& entityName, co::int32 version ) 
		{
			executeOrThrow( SpaceSaverSQLQueries::insertEntity(entityName, version) );
			return getEntityIdByName(entityName, version);
		}
		
		co::int32 insertField( const string& fieldName, const string& fieldType, co::int32 entityId )
		{
			executeOrThrow( SpaceSaverSQLQueries::insertField(fieldName, entityId, fieldType) );
			return getFieldIdByName(fieldName, entityId);
		}
	
		co::int32 insertObject( co::int32 entityId )
		{
			executeOrThrow( SpaceSaverSQLQueries::insertObject( entityId ) );

			IResultSet* rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectLastInsertedObject() );
			if(!rs->next())
			{
				throw ca::InvalidSpaceFileException();
			}
			std::string resultValue (rs->getValue(0));
			rs->finalize();
			delete rs;
			return atoi(resultValue.c_str());
		}

		co::int32 insertFieldValue( co::int32 fieldId, co::int32 objId, co::int32 fieldValueVersion, const string& fieldValue )
		{
			executeOrThrow( SpaceSaverSQLQueries::insertFieldValue( fieldId, objId, fieldValueVersion, fieldValue ) );
			return 1;
		}
	
		const ca::EntityBean& getEntityOfObject( co::int32 objectId )
		{
			ca::IResultSet* rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectEntityFromObject( objectId ) );
				
			if(rs->next())
			{
				std::string fullName (rs->getValue(0));
				_entityBean.fullName = fullName;
				std::string idStr (rs->getValue(1));
				_entityBean.id = atoi(idStr.c_str());
				rs->finalize();
				delete rs;
				return _entityBean;
			}
			else
			{
				rs->finalize();
				delete rs;
				stringstream ss;
				ss << "Not such object, id=" << objectId;
				throw InvalidSpaceFileException(ss.str());
			}
			

		
		}
		
		co::int32 getFieldIdByName( const string& fieldName, co::int32 entityId )
		{
			ca::IResultSet* rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectFieldIdByName( fieldName, entityId ) );
			
			int result = -1;

			if(rs->next())
			{
				std::string idStr(rs->getValue(0));
				result = atoi( idStr.c_str() );
			}
			rs->finalize();
			delete rs;

			return result;
		}

		void getFieldValues( co::int32 objectId, co::int32 fieldValueVersion, std::vector<ca::FieldValueBean>& values )
		{
			values.clear();

			ca::IResultSet* rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectFieldValues( objectId, 1, fieldValueVersion ) );
				
			while( rs->next() )
			{
				ca::FieldValueBean bean;
				std::string fieldName(rs->getValue(0));
				bean.fieldName = fieldName;
				std::string fieldValue(rs->getValue(1));
				bean.fieldValue = fieldValue;
				values.push_back( bean );
			}
			rs->finalize();
			delete rs;
		}

		co::int32 getEntityIdByName(const std::string& entityName, co::int32 calciumModelId)
		{
			ca::IResultSet* rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectEntityIdByName( entityName, calciumModelId ) );
			if(rs->next())
			{
				std::string entityIdStr(rs->getValue(0));
				rs->finalize();
				delete rs;
				return atoi( entityIdStr.c_str() );
			}
			rs->finalize();
			delete rs;
			return -1;
		}

		const std::string& getName() 
		{
			return _fileName;
		}

	//! Set the service at receptacle 'connection', of type ca.IDBConnection.
		void setName( const std::string& fileName )
		{
			_fileName = fileName;
		}

	private:

		std::string _fileName;
		ca::SQLiteDBConnection _db;
		ca::EntityBean _entityBean;

		void createTables()
		{
			try{
				_db.execute("BEGIN TRANSACTION");

				_db.execute("PRAGMA foreign_keys = 1");

				_db.execute( SpaceSaverSQLQueries::createTableCalciumModel() );

				_db.execute( SpaceSaverSQLQueries::createTableEntity() );
				
				_db.execute( SpaceSaverSQLQueries::createTableField() );

				_db.execute( SpaceSaverSQLQueries::createTableObject() );
					
				_db.execute( SpaceSaverSQLQueries::createTableFieldValues() );
				
				_db.execute("COMMIT TRANSACTION");
			}
			catch(ca::DBException e)
			{
				_db.execute("ROLLBACK TRANSACTION");
				throw e;
			}
		}

		void insertModel()
		{
			ca::IResultSet* rs = _db.executeQuery( "SELECT CAMODEL_ID, CAMODEL_CONTENT FROM CALCIUM_MODEL" );
			
			if( !rs->next() )
			{
				_db.execute( SpaceSaverSQLQueries::insertCalciumModel( string("dummy"), 1 ) );
			}

			delete rs;
		}

		ca::IResultSet* executeQueryOrThrow( std::string sql )
		{
			try
			{
				ca::IResultSet* rs = _db.executeQuery( sql );
				return rs;
			}
			catch ( ca::DBException e )
			{
				throw ca::InvalidSpaceFileException("Unexpected database query exception");
			}		
				
		}

		void executeOrThrow( std::string sql )
		{
			try
			{
				_db.execute( sql );
					
			}
			catch ( ca::DBException e )
			{
				throw ca::InvalidSpaceFileException("Unexpected database query exception");
			}	
		}

	};
	CORAL_EXPORT_COMPONENT(DBSpaceFile, DBSpaceFile);
}