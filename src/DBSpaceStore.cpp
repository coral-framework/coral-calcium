#include "DBSpaceStore_Base.h"
#include "DBSpaceStoreQueries.h"

#include <co/RefPtr.h>
#include <co/Range.h>
#include <co/IllegalArgumentException.h>

#include <ca/StoredFieldValue.h>
#include <ca/StoredType.h>
#include <ca/StoredField.h>

#include <ca/IOException.h>

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
					throw ca::IOException( e.what() );
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
				throw ca::IOException( e.what() );
			}
		}

		co::uint32 getOrAddType( const string& typeName, co::uint32 version ) 
		{
			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectTypeIdByName(typeName, version) );
			if( !rs->next() )
			{
				rs->finalize();
				delete rs;
				
				executeOrThrow( DBSpaceStoreQueries::insertType( typeName, version ) );
				rs = executeQueryOrThrow( DBSpaceStoreQueries::selectTypeIdByName( typeName, version ) );
				rs->next();
			}

			int id = atoi( rs->getValue(0).c_str() );
			rs->finalize();
			delete rs;
			return id;

		}
		
		co::uint32 addField( co::uint32 typeId, const string& fieldName, co::uint32 fieldTypeId )
		{
			executeOrThrow( DBSpaceStoreQueries::insertField( typeId, fieldName, fieldTypeId ) );

			return getFieldIdByName(fieldName, typeId);
		}
	
		co::uint32 addObject( co::uint32 typeId )
		{
			executeOrThrow( DBSpaceStoreQueries::insertObject( typeId ) );

			IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectLastInsertedObject() );
			if(!rs->next())
			{
				throw ca::IOException("Failed to add object");
			}
			co::uint32 resultId = atoi( rs->getValue(0).c_str() );
			rs->finalize();
			delete rs;
			return resultId;
		}

		void addValues( co::uint32 objId, co::uint32 revision, co::Range<const ca::StoredFieldValue> values )
		{
			for( int i = 0; i < values.getSize(); i++ )
			{
				executeOrThrow( DBSpaceStoreQueries::insertFieldValue( values[i].fieldId, objId, revision, values[i].value ) );
			}
		}
	
		co::uint32 getObjectType( co::uint32 objectId )
		{
			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectEntityFromObject( objectId ) );
				
			if(rs->next())
			{
				int id = atoi(rs->getValue(0).c_str());
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
				throw IOException(ss.str());
			}
					
		}
		
		void getValues( co::uint32 objectId, co::uint32 revision, std::vector<ca::StoredFieldValue>& values )
		{
			values.clear();

			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectFieldValues( objectId, revision ) );
				
			while( rs->next() )
			{
				ca::StoredFieldValue sfv;
				sfv.fieldId = atoi( rs->getValue( 0 ).c_str() );//fieldName;
				sfv.value = rs->getValue( 1 );
				values.push_back( sfv );
			}
			rs->finalize();
			delete rs;
		}

		void getStoredType( co::uint32 typeId, ca::StoredType& storedType )
		{
			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectTypeById( typeId ) );
			bool first = true;
			while( rs->next() )
			{
				if( first )
				{
					storedType.fields.clear();
					storedType.typeId = typeId;
					storedType.typeName = std::string( rs->getValue( 1 ) );
					first = false;
				}

				StoredField sf;
				sf.fieldId = atoi( rs->getValue( 2 ).c_str() );
				sf.fieldName = rs->getValue( 3 );
				sf.typeId = atoi( rs->getValue( 4 ).c_str() );
				
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

		co::uint32 getCurrentRevision()
		{
			return _currentRevision;
		}

		void setCurrentRevision( co::uint32 currentRevision )
		{
			if( currentRevision > _latestRevision || currentRevision == 0 )
			{
				throw co::IllegalArgumentException( "invalid revision" );
			}
			_currentRevision = currentRevision;
		}
		
		co::uint32 getLatestRevision()
		{
			return _currentRevision;
		}

	private:

		std::string _fileName;
		ca::SQLiteDBConnection _db;
		co::uint32 _currentRevision;
		co::uint32 _latestRevision;
		
		co::uint32 getFieldIdByName( const string& fieldName, co::uint32 entityId )
		{
			ca::IResultSet* rs = executeQueryOrThrow( DBSpaceStoreQueries::selectFieldIdByName( fieldName, entityId ) );
			
			int result = -1;

			if( rs->next() )
			{
				result = atoi( rs->getValue(0).c_str() );
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
				throw ca::IOException( e.what() );
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
				throw ca::IOException( "Unexpected database query exception" );
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
				throw ca::IOException( "Unexpected database query exception" );
			}	
		}

	};
	CORAL_EXPORT_COMPONENT(DBSpaceStore, DBSpaceStore);
}