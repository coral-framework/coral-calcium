#include "DBSpaceStore_Base.h"
#include "DBSpaceStoreQueries.h"

#include <co/RefPtr.h>
#include <co/Range.h>

#include <ca/InvalidSpaceFileException.h>


#include <ca/StoredFieldValue.h>
#include <ca/StoredType.h>
#include <ca/StoredField.h>

#include "DBException.h"

#include "SQLiteDBConnection.h"
#include "IResultSet.h"
#include "SQLiteResultSet.h"

#include <string>
using namespace std;

namespace ca {

	class DBSpaceStore : public DBSpaceStore_Base
	{
	public:
		DBSpaceStore()
		{
		}

		virtual ~DBSpaceStore()
		{
			close();
		}

		void open()
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
		}

		void close()
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

		co::int32 getOrAddType( const string& typeName, co::int32 version ) 
		{
			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectTypeIdByName(typeName, version) );
			if( !rs->next() )
			{
				rs->finalize();
				delete rs;
				
				executeOrThrow( DBSpaceStoreQueries::insertType(typeName, version) );
				rs = executeQueryOrThrow( DBSpaceStoreQueries::selectTypeIdByName(typeName, version) );
			}

			int id = atoi( rs->getValue(0).c_str() );
			return id;

		}
		
		co::int32 addField( co::int32 typeId, const string& fieldName, co::int32 fieldTypeId )
		{
			executeOrThrow( DBSpaceStoreQueries::insertField( typeId, fieldName, fieldTypeId ) );

			return getFieldIdByName(fieldName, typeId);
		}
	
		co::int32 addObject( co::int32 typeId )
		{
			executeOrThrow( DBSpaceStoreQueries::insertObject( typeId ) );

			IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectLastInsertedObject() );
			if(!rs->next())
			{
				throw ca::InvalidSpaceFileException();
			}
			std::string resultValue (rs->getValue(0));
			rs->finalize();
			delete rs;
			return atoi(resultValue.c_str());
		}

		void addValues( co::int32 objId, co::int32 revision, co::Range<const ca::StoredFieldValue> values )
		{
			for( int i = 0; i < values.getSize(); i++ )
			{
				executeOrThrow( DBSpaceStoreQueries::insertFieldValue( values[i].fieldId, objId, revision, values[i].value ) );
			}
		}
	
		co::int32 getObjectType( co::int32 objectId )
		{
			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectEntityFromObject( objectId ) );
				
			if(rs->next())
			{
				std::string idStr (rs->getValue(1));
				int id = atoi(idStr.c_str());
				rs->finalize();
				delete rs;
				return id;
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
		
		void getValues( co::int32 objectId, co::int32 revision, std::vector<ca::StoredFieldValue>& values )
		{
			values.clear();

			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectFieldValues( objectId, revision ) );
				
			while( rs->next() )
			{
				ca::StoredFieldValue sfv;
				sfv.fieldId = atoi( rs->getValue(0).c_str() );//fieldName;
				std::string fieldValue(rs->getValue(1));
				sfv.value = fieldValue;
				values.push_back( sfv );
			}
			rs->finalize();
			delete rs;
		}

		void getStoredType( co::int32 typeId, ca::StoredType& storedType )
		{
			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectTypeById( typeId ) );
			bool first = true;
			while(rs->next())
			{
				if( first )
				{
					storedType.fields.clear();
					storedType.typeId = typeId;
					storedType.typeName = std::string(rs->getValue(1));
					first = false;
				}
				StoredField sf;
				sf.fieldId = atoi( rs->getValue(2).c_str() );

				std::string fieldName( rs->getValue(3) );
				sf.fieldName = fieldName;

				sf.typeId = atoi( rs->getValue(4).c_str() );
				storedType.fields.push_back( sf );
			}
			rs->finalize();
			delete rs;
		}

		const std::string& getName() 
		{
			return _fileName;
		}

		void setName( const std::string& fileName )
		{
			_fileName = fileName;
		}

		co::int32 getCurrentRevision()
		{
			return _currentRevision;
		}

		void setCurrentRevision( co::int32 currentRevision )
		{
			if( currentRevision > _latestRevision )
			{
				//throw something
			}
			_currentRevision = currentRevision;
		}
		
		co::int32 getLatestRevision()
		{
			return _currentRevision;
		}

	private:

		std::string _fileName;
		ca::SQLiteDBConnection _db;
		co::int32 _currentRevision;
		co::int32 _latestRevision;
		
		co::int32 getFieldIdByName( const string& fieldName, co::int32 entityId )
		{
			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectFieldIdByName( fieldName, entityId ) );
			
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

		void createTables()
		{
			try{
				_db.execute("BEGIN TRANSACTION");

				_db.execute("PRAGMA foreign_keys = 1");


				_db.execute( DBSpaceStoreQueries::createTableType() );
				
				_db.execute( DBSpaceStoreQueries::createTableField() );

				_db.execute( DBSpaceStoreQueries::createTableObject() );
					
				_db.execute( DBSpaceStoreQueries::createTableFieldValues() );
				
				_db.execute("COMMIT TRANSACTION");
			}
			catch(ca::DBException e)
			{
				_db.execute("ROLLBACK TRANSACTION");
				throw e;
			}
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
	CORAL_EXPORT_COMPONENT(DBSpaceStore, DBSpaceStore);
}