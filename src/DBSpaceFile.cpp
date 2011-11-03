#include "DBSpaceFile_Base.h"
#include "SpaceSaverSQLQueries.h"

#include <co/RefPtr.h>
#include <co/Range.h>

#include <ca/IDBConnection.h>
#include <ca/InvalidSpaceFileException.h>
#include <ca/DBException.h>
#include <ca/IResultSet.h>

#include <ca/FieldValueBean.h>
#include <ca/EntityBean.h>


#include <string>
using namespace std;

namespace ca {

	class DBSpaceFile : public DBSpaceFile_Base
	{
	public:
		DBSpaceFile()
		{
		}

		void initialize()
		{
			try
			{
				if( !_db->isConnected() )
				{
					_db->open();
				}
				createTables();
				insertModel();
			}
			catch( ca::DBException e )
			{
				std::string msg = e.getMessage();
				msg.c_str();
			}
		}

		void finalize()
		{
			try
			{
				_db->close();
			}
			catch( ca::DBException e )
			{
				//throw something
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

			co::RefPtr<IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectLastInsertedObject() );
			if(!rs->next())
			{
				throw ca::InvalidSpaceFileException();
			}

			return atoi(rs->getValue(0).c_str());
		}

		co::int32 insertFieldValue( co::int32 fieldId, co::int32 objId, co::int32 fieldValueVersion, const string& fieldValue )
		{
			executeOrThrow( SpaceSaverSQLQueries::insertFieldValue( fieldId, objId, fieldValueVersion, fieldValue ) );
			return 1;
		}
	
		const ca::EntityBean& getEntityOfObject( co::int32 objectId )
		{
			co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectEntityFromObject( objectId ) );
				
			if(rs->next())
			{
				_entityBean.fullName = rs->getValue(0);
				_entityBean.id = atoi(rs->getValue(1).c_str());
				rs->finalize();
				return _entityBean;
			}
			rs->finalize();
			return ca::EntityBean();
			
		}
		
		co::int32 getFieldIdByName( const string& fieldName, co::int32 entityId )
		{
			co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectFieldIdByName( fieldName, entityId ) );
			
			int result = -1;

			if(rs->next())
			{
				result = atoi( rs->getValue(0).c_str() );
			}
			rs->finalize();

			return result;
		}

		void getFieldValues( co::int32 objectId, co::int32 fieldValueVersion, std::vector<ca::FieldValueBean>& values )
		{
			values.clear();

			co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectFieldValues( objectId, 1, fieldValueVersion ) );
				
			while( rs->next() )
			{
				ca::FieldValueBean bean;
				bean.fieldName = rs->getValue(0);
				bean.fieldValue = rs->getValue(1);
				values.push_back( bean );
			}
			rs->finalize();
		}

		co::int32 getEntityIdByName(const std::string& entityName, co::int32 calciumModelId)
		{
			co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectEntityIdByName( entityName, calciumModelId ) );
			if(rs->next())
			{
				const char* entityIdStr = rs->getValue(0).c_str();
				rs->finalize();
				return atoi( entityIdStr );
			}
			rs->finalize();	
			return -1;
		}

		ca::IDBConnection* getConnectionService() 
		{
			return _db;
		}

	//! Set the service at receptacle 'connection', of type ca.IDBConnection.
		void setConnectionService( ca::IDBConnection* connection )
		{
			assert( connection );
			_db = connection;
		}
	private:

		ca::IDBConnection* _db;
		ca::EntityBean _entityBean;

		void createTables()
		{
			try{
				_db->execute("BEGIN TRANSACTION");

				_db->execute("PRAGMA foreign_keys = 1");

				_db->execute( SpaceSaverSQLQueries::createTableCalciumModel() );

				_db->execute( SpaceSaverSQLQueries::createTableEntity() );
				
				_db->execute( SpaceSaverSQLQueries::createTableField() );

				_db->execute( SpaceSaverSQLQueries::createTableObject() );
					
				_db->execute( SpaceSaverSQLQueries::createTableFieldValues() );
				
				_db->execute("COMMIT TRANSACTION");
			}
			catch(ca::DBException e)
			{
				_db->execute("ROLLBACK TRANSACTION");
				throw e;
			}
		}

		void insertModel()
		{
			co::RefPtr<ca::IResultSet> rs = _db->executeQuery( "SELECT CAMODEL_ID, CAMODEL_CONTENT FROM CALCIUM_MODEL" );
			
			if( !rs->next() )
			{
				_db->execute( SpaceSaverSQLQueries::insertCalciumModel( string("dummy"), 1 ) );
			}
		}

		co::RefPtr<ca::IResultSet> executeQueryOrThrow( std::string sql )
		{
			try
			{
				co::RefPtr<ca::IResultSet> rs = _db->executeQuery( sql );
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
				_db->execute( sql );
					
			}
			catch ( ca::DBException e )
			{
				throw ca::InvalidSpaceFileException("Unexpected database query exception");
			}	
		}

	};
	CORAL_EXPORT_COMPONENT(DBSpaceFile, DBSpaceFile);
}