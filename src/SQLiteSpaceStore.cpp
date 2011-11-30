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

#include "SQLiteConnection.h"
#include "SQLiteResultSet.h"

#include <string>

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
		catch( ca::DBException& e )
		{
			try
			{
				_db.open();
			}
			catch( ca::DBException& e )
			{
				throw ca::IOException( e.what() );
			}
		}

		try
		{
			createTables();
			fillLatestRevision();
		}
		catch( ca::DBException& e )
		{
			throw ca::IOException( e.what() );
		}

	}

	void close()
	{
		try
		{
			_db.close();
		}
		catch( ca::DBException& e )
		{
			throw ca::IOException( e.what() );
		}
	}

	void beginChanges()
	{
		_latestRevision++;
		_currentRevision = _latestRevision;

		if( _currentRevision == 1 )
		{
			_firstObject = true;
		}

	}

	void endChanges()
	{
		executeOrThrow( DBSpaceStoreQueries::insertNewRevision( _rootObjectId, _currentRevision ) );
	}

	co::uint32 getRootObject( co::uint32 revision )
	{
		return getRootObjectForRevision( revision );
	}

	co::uint32 getOrAddType( const std::string& typeName, co::uint32 version ) 
	{
		ca::SQLiteResultSet rs; 
		executeQueryOrThrow( DBSpaceStoreQueries::selectTypeIdByName(typeName, version), rs );
		if( !rs.next() )
		{
			rs.finalize();
			executeOrThrow( DBSpaceStoreQueries::insertType( typeName, version ) );
			executeQueryOrThrow( DBSpaceStoreQueries::selectTypeIdByName( typeName, version ), rs );
			rs.next();
		}

		int id = atoi( rs.getValue(0).c_str() );
		rs.finalize();
		
		return id;

	}
		
	co::uint32 addField( co::uint32 typeId, const std::string& fieldName, co::uint32 fieldTypeId )
	{
		executeOrThrow( DBSpaceStoreQueries::insertField( typeId, fieldName, fieldTypeId ) );

		return getFieldIdByName(fieldName, typeId);
	}
	
	co::uint32 addObject( co::uint32 typeId )
	{

		executeOrThrow( DBSpaceStoreQueries::insertObject( typeId ) );

		ca::SQLiteResultSet rs; 
		executeQueryOrThrow( DBSpaceStoreQueries::selectLastInsertedObject(), rs );
		if(!rs.next())
		{
			throw ca::IOException("Failed to add object");
		}
		co::uint32 resultId = atoi( rs.getValue(0).c_str() );
		rs.finalize();

		if( _firstObject )
		{
			_rootObjectId = resultId;
			_firstObject = false;
		}
		return resultId;
	}

	void addValues( co::uint32 objId, co::Range<const ca::StoredFieldValue> values )
	{
		for( int i = 0; i < values.getSize(); i++ )
		{
			executeOrThrow( DBSpaceStoreQueries::insertFieldValue( values[i].fieldId, objId, _currentRevision, values[i].value ) );
		}
	}
	
	co::uint32 getObjectType( co::uint32 objectId )
	{
		ca::SQLiteResultSet rs; 
		executeQueryOrThrow( DBSpaceStoreQueries::selectEntityFromObject( objectId ), rs );
				
		if(rs.next())
		{
			int id = atoi(rs.getValue(0).c_str());
			rs.finalize();
			return id;
		}
		else
		{
			rs.finalize();
			std::stringstream ss;
			ss << "Not such object, id=" << objectId;
			throw IOException( ss.str() );
		}
					
	}
		
	void getValues( co::uint32 objectId, co::uint32 revision, std::vector<ca::StoredFieldValue>& values )
	{
		values.clear();
		ca::SQLiteResultSet rs;
		executeQueryOrThrow( DBSpaceStoreQueries::selectFieldValues( objectId, revision ), rs );
				
		while( rs.next() )
		{
			ca::StoredFieldValue sfv;
			sfv.fieldId = atoi( rs.getValue( 0 ).c_str() ); //fieldName;
			sfv.value = rs.getValue( 1 );
			values.push_back( sfv );
		}
		rs.finalize();
	}

	void getType( co::uint32 typeId, ca::StoredType& storedType )
	{
		ca::SQLiteResultSet rs;
		executeQueryOrThrow( DBSpaceStoreQueries::selectTypeById( typeId ), rs );
		
		bool first = true;
		while( rs.next() )
		{
			if( first )
			{
				storedType.fields.clear();
				storedType.typeId = typeId;
				storedType.typeName = std::string( rs.getValue( 1 ) );
				first = false;
			}

			StoredField sf;
			sf.fieldId = atoi( rs.getValue( 2 ).c_str() );
			sf.fieldName = rs.getValue( 3 );
			sf.typeId = atoi( rs.getValue( 4 ).c_str() );
				
			storedType.fields.push_back( sf );
		}
		rs.finalize();
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

		_rootObjectId = getRootObjectForRevision( currentRevision );
			

		_currentRevision = currentRevision;
	}
		
	co::uint32 getLatestRevision()
	{
		return _latestRevision;
	}

private:

	std::string _fileName;
	ca::SQLiteConnection _db;
	co::uint32 _currentRevision;
	co::uint32 _latestRevision;
	co::uint32 _rootObjectId;
	bool _firstObject;
		
	co::uint32 getRootObjectForRevision( co::uint32 revision )
	{
		co::uint32 rootObject = 0;
		ca::SQLiteResultSet rs;
		executeQueryOrThrow( DBSpaceStoreQueries::selectObjectIdForRevision( revision ), rs );
		
		if( rs.next() )
		{
			rootObject = atoi( rs.getValue(0).c_str() );
			rs.finalize();
		}
		return rootObject;
	}

	co::uint32 getFieldIdByName( const std::string& fieldName, co::uint32 entityId )
	{
		ca::SQLiteResultSet rs;
		executeQueryOrThrow( DBSpaceStoreQueries::selectFieldIdByName( fieldName, entityId ), rs );			
		int result = -1;

		if( rs.next() )
		{
			result = atoi( rs.getValue(0).c_str() );
		}
		rs.finalize();

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

			_db.execute( DBSpaceStoreQueries::createTableSpace() );
				
			_db.execute("COMMIT TRANSACTION");
		}
		catch( ca::DBException& e )
		{
			_db.execute("ROLLBACK TRANSACTION");
			throw ca::IOException( e.what() );
		}
	}

	void fillLatestRevision()
	{
		ca::SQLiteResultSet rs;
		executeQueryOrThrow( DBSpaceStoreQueries::selectLatestRevision(), rs );
			
		if( rs.next() )
		{
			_latestRevision = atoi( rs.getValue(0).c_str() );
			_rootObjectId = atoi( rs.getValue(1).c_str() );
		}
		else
		{
			_latestRevision = 0;
		}
		rs.finalize();
		_currentRevision = _latestRevision;

	}

	void executeQueryOrThrow( const std::string& sql, ca::SQLiteResultSet& rs )
	{
		try
		{
			_db.executeQuery( sql, rs );
		}
		catch ( ca::DBException& e )
		{
			throw ca::IOException( "Unexpected database query exception" );
		}		
				
	}

	void executeOrThrow( const std::string& sql )
	{
		try
		{
			_db.execute( sql );
					
		}
		catch ( ca::DBException& e )
		{
			throw ca::IOException( "Unexpected database query exception" );
		}	
	}

};
CORAL_EXPORT_COMPONENT( DBSpaceStore, DBSpaceStore );

} // namespace ca