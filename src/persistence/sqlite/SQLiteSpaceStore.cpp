/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SQLite.h"
#include "SQLiteSpaceStore_Base.h"

#include <co/RefPtr.h>

#include <ca/IOException.h>


namespace ca {

class SQLiteSpaceStore : public SQLiteSpaceStore_Base
{
public:
	SQLiteSpaceStore()
	{
		_inTransaction = false;
		_firstObject = false;
		_startedRevision = false;
		_latestRevision = 0;
	}

	virtual ~SQLiteSpaceStore()
	{
		close();
	}

	void open()
	{
		assert( !_fileName.empty() );

		_db.open( _fileName );

		if( checkEmptyValidDatabase() )
		{
			createTables();
		}
		fillLatestRevision();

	}

	void close()
	{
		_db.close();
	}

	void beginChanges()
	{
		_db.prepare( "BEGIN TRANSACTION" ).execute();
		_inTransaction = true;
	}

	void commitChanges( const std::string& updates )
	{
		checkBeginTransaction();

		if( _startedRevision )
		{
			ca::SQLiteStatement stmt = _db.prepare( "INSERT INTO SPACE VALUES (?, ?, datetime('now'), ? )" );

			stmt.bind( 1, _rootObjectId );
			stmt.bind( 2, _latestRevision );
			stmt.bind( 3, updates );

			stmt.execute();
			_startedRevision = false;

			stmt.finalize();
		}

		_db.prepare( "COMMIT TRANSACTION" ).execute(); 
		_inTransaction = false;

	}

	void discardChanges()
	{
		checkBeginTransaction();
		
		_db.prepare("ROLLBACK TRANSACTION").execute();
		_inTransaction = false;
		if( _startedRevision )
		{
			_latestRevision--;
			_startedRevision = false;
		}
	}

	co::uint32 getRootObject( co::uint32 revision )
	{
		co::uint32 rootObject = 0;
		ca::SQLiteStatement stmt = _db.prepare( "SELECT ROOT_OBJECT_ID FROM SPACE WHERE REVISION = ?" );

		stmt.bind( 1, revision );

		ca::SQLiteResult rs = stmt.query();

		if( rs.next() )
		{
			rootObject = rs.getUint32(0);
			stmt.finalize();
		}
		return rootObject;
	}

	void setRootObject( co::uint32 objectId )
	{
		checkBeginTransaction();
		_rootObjectId = objectId;
	}

	void getUpdates( co::uint32 revision, std::string& updates )
	{
		ca::SQLiteStatement stmt = _db.prepare( "SELECT UPDATES_APPLIED FROM SPACE WHERE REVISION = ?" );

		stmt.bind( 1, revision );

		ca::SQLiteResult rs = stmt.query();

		if( rs.next() )
		{
			updates = rs.getString( 0 );
			stmt.finalize();
		}
	}

	co::uint32 addObject( const std::string& typeName )
	{
		return addService( typeName, 0 );
	}

	co::uint32 addService( const std::string& typeName, co::uint32 providerId )
	{
		checkBeginTransaction();
		checkGenerateRevision();

		ca::SQLiteStatement stmtMaxObj = _db.prepare( "SELECT MAX(OBJECT_ID) FROM FIELD_VALUE" );
		ca::SQLiteResult rs = stmtMaxObj.query();

		co::uint32 newObjectId;

		if( !rs.next() )
		{
			newObjectId = 1;
		}
		else
		{
			newObjectId = rs.getUint32(0) + 1;
		}
		
		std::vector<std::string> fieldNames;
		std::vector<std::string> values;

		if( providerId > 0 )
		{
			std::stringstream ss;
			ss << providerId;

			fieldNames.push_back( "_provider" );
			values.push_back( ss.str() );

		}

		fieldNames.push_back( "_type" );

		values.push_back( typeName );

		addValues( newObjectId, fieldNames, values );

		if( _firstObject )
		{
			_rootObjectId = newObjectId;
			_firstObject = false;
		}

		return newObjectId;

	}

	void addValues( co::uint32 objId, co::Range<std::string> fieldNames, co::Range<std::string> values )
	{
		checkBeginTransaction();
		checkGenerateRevision();

		ca::SQLiteStatement stmt = _db.prepare( "INSERT INTO FIELD_VALUE (FIELD_NAME, OBJECT_ID, REVISION, VALUE)\
												VALUES (?, ?, ?, ?)" );

		for( int i = 0; i < values.getSize(); i++ )
		{
			stmt.reset();
			stmt.bind( 1, fieldNames[i] );
			stmt.bind( 2, objId );
			stmt.bind( 3, _latestRevision );
			stmt.bind( 4, values[i] );

			stmt.execute();
		}
		stmt.finalize();

	}

	void getObjectType( co::uint32 objectId, co::uint32 revision, std::string& typeName )
	{
		ca::SQLiteStatement stmt = _db.prepare( "SELECT FV.VALUE FROM (SELECT OBJECT_ID, MAX(REVISION) AS LATEST_REVISION, FIELD_NAME, \
												VALUE FROM FIELD_VALUE WHERE REVISION <= ? GROUP BY OBJECT_ID, FIELD_NAME ) FV\
												WHERE FV.FIELD_NAME = '_type' AND OBJECT_ID = ? GROUP BY FV.FIELD_NAME, FV.OBJECT_ID \
												ORDER BY FV.OBJECT_ID" );

		stmt.bind( 1, revision );
		stmt.bind( 2, objectId );

		ca::SQLiteResult rs = stmt.query();
				
		if( rs.next() )
		{
			typeName = rs.getString( 0 );
			stmt.finalize();
		}
		else
		{
			stmt.finalize();
			CORAL_THROW( ca::IOException, "Not such object, id =" << objectId );
		}
					
	}
		
	void getValues( co::uint32 objectId, co::uint32 revision, std::vector<std::string>& fieldNames, std::vector<std::string>& values )
	{
		fieldNames.clear();
		values.clear();
		
		ca::SQLiteStatement stmt = _db.prepare( "SELECT FV.FIELD_NAME, FV.VALUE FROM (SELECT OBJECT_ID, MAX(REVISION) AS LATEST_REVISION, FIELD_NAME, \
												VALUE FROM FIELD_VALUE WHERE REVISION <= ? GROUP BY OBJECT_ID, FIELD_NAME ) FV\
												WHERE FV.FIELD_NAME <> '_type' AND FV.FIELD_NAME <> '_provider' AND OBJECT_ID = ? GROUP BY FV.FIELD_NAME, FV.OBJECT_ID \
												ORDER BY FV.OBJECT_ID" );

		stmt.bind( 1, revision );
		stmt.bind( 2, objectId );
		ca::SQLiteResult rs = stmt.query();
				
		while( rs.next() )
		{
			fieldNames.push_back( rs.getString( 0 ) );
			values.push_back( rs.getString( 1 ) );
		}
		stmt.finalize();
	}
	
	std::string getName() 
	{
		return _fileName;
	}

	void setName( const std::string& fileName )
	{
		_fileName = fileName;
	}
	
	co::uint32 getLatestRevision()
	{
		if( _startedRevision )
		{
			return _latestRevision-1;
		}
		return _latestRevision;
	}

	co::uint32 getServiceProvider( co::uint32 serviceId, co::uint32 revision )
	{
		co::uint32 object = 0;

		ca::SQLiteStatement stmt = _db.prepare( "SELECT FV.VALUE FROM (SELECT OBJECT_ID, MAX(REVISION) AS LATEST_REVISION, FIELD_NAME, \
												VALUE FROM FIELD_VALUE WHERE REVISION <= ? GROUP BY OBJECT_ID, FIELD_NAME ) FV\
												WHERE FV.FIELD_NAME = '_provider' AND OBJECT_ID = ? GROUP BY FV.FIELD_NAME, FV.OBJECT_ID \
												ORDER BY FV.OBJECT_ID" );


		stmt.bind( 1, revision );
		stmt.bind( 2, serviceId );

		ca::SQLiteResult rs = stmt.query();

		if( rs.next() )
		{
			object = rs.getUint32(0);
			stmt.finalize();
		}
		return object;
	}

private:
	void checkGenerateRevision()
	{
		if( !_startedRevision )
		{
			_latestRevision++;
			if( _latestRevision == 1 )
			{
				_firstObject = true;
			}
			_startedRevision = true;
		}
	}

	void checkBeginTransaction()
	{
		if( !_inTransaction )
		{
			CORAL_THROW( ca::IOException, "Attempt to call a store modification routine without calling 'beginChanges' before." );
		}
	}

	bool checkEmptyValidDatabase()
	{
		ca::SQLiteStatement stmt = _db.prepare( "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name" );
		ca::SQLiteResult rs = stmt.query();
		
		bool valid = !rs.next();

		stmt.finalize();

		return valid;
	}

	void createTables()
	{
		try{
			_db.prepare("BEGIN TRANSACTION").execute();

			_db.prepare( "CREATE TABLE if not exists [FIELD_VALUE] (\
						 [FIELD_NAME] VARCHAR(128),\
						 [VALUE] TEXT  NULL,\
						 [REVISION] INTEGER NOT NULL,\
						 [OBJECT_ID] INTEGER NOT NULL,\
						 PRIMARY KEY (REVISION, OBJECT_ID, FIELD_NAME)\
						 );" ).execute();

			_db.prepare( "CREATE TABLE if not exists [SPACE] (\
						 [ROOT_OBJECT_ID] INTEGER NOT NULL,\
						 [REVISION] INTEGER  NOT NULL,\
						 [TIME] TEXT NOT NULL,\
						 [UPDATES_APPLIED] TEXT, \
						 UNIQUE( REVISION ));" ).execute();

			_db.prepare("COMMIT TRANSACTION").execute();
		}
		catch( ... )
		{
			_db.prepare("ROLLBACK TRANSACTION").execute();
			throw;
		}
	}

	void fillLatestRevision()
	{
		ca::SQLiteStatement stmt = _db.prepare( "SELECT MAX(REVISION), ROOT_OBJECT_ID FROM SPACE GROUP BY ROOT_OBJECT_ID" );
		ca::SQLiteResult rs = stmt.query();

		if( rs.next() )
		{
			_latestRevision = rs.getUint32(0);
			_rootObjectId = rs.getUint32(1);
		}
		else
		{
			_latestRevision = 0;
		}

	}

private:
	ca::SQLiteConnection _db;
	std::string _fileName;

	co::uint32 _latestRevision;

	co::uint32 _rootObjectId;
	bool _firstObject;
	bool _inTransaction;
	bool _startedRevision;
};

CORAL_EXPORT_COMPONENT( SQLiteSpaceStore, SQLiteSpaceStore );

} // namespace ca
