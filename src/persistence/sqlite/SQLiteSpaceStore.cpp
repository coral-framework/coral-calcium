/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SQLite.h"
#include "SQLiteSpaceStore_Base.h"

#include <co/RefPtr.h>
#include <co/IllegalArgumentException.h>

#include <ca/StoredFieldValue.h>
#include <ca/StoredType.h>
#include <ca/StoredField.h>

#include <ca/IOException.h>

namespace ca {

class SQLiteSpaceStore : public SQLiteSpaceStore_Base
{
public:
	SQLiteSpaceStore()
	{
		_inTransaction = false;
		_startedRevision = false;
	}

	virtual ~SQLiteSpaceStore()
	{
		close();
	}

	void open()
	{
		assert( !_fileName.empty() );

		try
		{
			_db.open( _fileName );
			createTables();
			fillLatestRevision();
		}
		catch( ca::SQLiteException& e )
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
		catch( ca::SQLiteException& e )
		{
			throw ca::IOException( e.what() );
		}
	}

	void beginChanges()
	{
		executeOrThrow( prepareOrThrow( "BEGIN TRANSACTION" ) );
		_inTransaction = true;

		if( _currentRevision == 1 )
		{
			_firstObject = true;
		}
	}

	void endChanges()
	{
		if( _startedRevision )
		{
			ca::SQLiteStatement stmt = prepareOrThrow( "INSERT INTO SPACE VALUES (?, ?, datetime('now') )" );

			stmt.bind( 1, _rootObjectId );
			stmt.bind( 2, _currentRevision );

			executeOrThrow( stmt );
			_startedRevision = false;

			stmt.finalize();
		}
		

		try 
		{
			executeOrThrow( prepareOrThrow( "COMMIT TRANSACTION" ) );
			_inTransaction = false;
		}
		catch( ca::IOException& e )
		{
			if( _inTransaction )
			{
				executeOrThrow(prepareOrThrow("ROLLBACK TRANSACTION"));
				_inTransaction = false;
			}
			
			throw e;
		}

	}

	co::uint32 getRootObject( co::uint32 revision )
	{
		return getRootObjectForRevision( revision );
	}

	co::uint32 getOrAddType( const std::string& typeName, co::uint32 version ) 
	{
		std::string typeQuery = "SELECT TYPE_ID FROM TYPE WHERE TYPE_NAME = ?  AND TYPE_VERSION = ?";
		ca::SQLiteStatement typeStmt = prepareOrThrow( typeQuery );

		typeStmt.bind( 1, typeName );
		typeStmt.bind( 2, version );

		ca::SQLiteResult rs = executeQueryOrThrow( typeStmt );
		co::uint32 id;
		if( !rs.next() )
		{
			checkBeginTransaction();
			typeStmt.reset();

			ca::SQLiteStatement stmt = prepareOrThrow( "INSERT INTO TYPE (TYPE_NAME, TYPE_VERSION) VALUES (?, ?)" );
			stmt.bind( 1, typeName );
			stmt.bind( 2, version );
			
			executeOrThrow( stmt );
			stmt.finalize();
			
			ca::SQLiteResult rs = executeQueryOrThrow( typeStmt );
			rs.next();
			id = rs.getUint32(0);
		}
		else
		{
			id = rs.getUint32(0);
		}

		typeStmt.finalize();

		return id;
	}
		
	co::uint32 addField( co::uint32 typeId, const std::string& fieldName, co::uint32 fieldTypeId )
	{
		checkBeginTransaction();
		ca::SQLiteStatement stmt = prepareOrThrow( "INSERT INTO FIELD (TYPE_ID, FIELD_NAME, FIELD_TYPE_ID) VALUES ( ?, ?, ? );" );

		stmt.bind( 1, typeId );
		stmt.bind( 2, fieldName );
		stmt.bind( 3, fieldTypeId );

		executeOrThrow( stmt );
		stmt.finalize();

		return getFieldIdByName(fieldName, typeId);
	}
	
	co::uint32 addObject( co::uint32 typeId )
	{
		checkBeginTransaction();
		checkGenerateRevision();

		ca::SQLiteStatement stmt = prepareOrThrow( "INSERT INTO OBJECT (TYPE_ID) VALUES ( ? );" );
		stmt.bind( 1, typeId );
		executeOrThrow( stmt );

		ca::SQLiteStatement stmtMaxObj = prepareOrThrow( "SELECT MAX(OBJECT_ID) FROM OBJECT" );

		ca::SQLiteResult rs = stmtMaxObj.query();

		if(!rs.next())
		{
			throw ca::IOException("Failed to add object");
		}
		co::uint32 resultId = rs.getUint32(0);

		if( _firstObject )
		{
			_rootObjectId = resultId;
			_firstObject = false;
		}
		//stmt.finalize();
		return resultId;
	}

	void addValues( co::uint32 objId, co::Range<const ca::StoredFieldValue> values )
	{
		checkBeginTransaction();
		checkGenerateRevision();

		ca::SQLiteStatement stmt = prepareOrThrow( "INSERT INTO FIELD_VALUE (FIELD_ID, OBJECT_ID, REVISION, VALUE)\
										 VALUES (?, ?, ?, ?)" );

		for( int i = 0; i < values.getSize(); i++ )
		{
			stmt.reset();
			stmt.bind( 1, values[i].fieldId );
			stmt.bind( 2, objId );
			stmt.bind( 3, _currentRevision );
			stmt.bind( 4, values[i].value );

			executeOrThrow( stmt );
		}
		stmt.finalize();
	}
	
	co::uint32 getObjectType( co::uint32 objectId )
	{
		ca::SQLiteStatement stmt = prepareOrThrow( "SELECT O.TYPE_ID FROM OBJECT O WHERE O.OBJECT_ID = ?" );

		stmt.bind( 1, objectId );

		ca::SQLiteResult rs = executeQueryOrThrow( stmt );
				
		if( rs.next() )
		{
			co::uint32 id = rs.getUint32(0);
			stmt.finalize();
			return id;
		}
		else
		{
			stmt.finalize();
			std::stringstream ss;
			ss << "Not such object, id=" << objectId;
			throw IOException( ss.str() );
		}
					
	}
		
	void getValues( co::uint32 objectId, co::uint32 revision, std::vector<ca::StoredFieldValue>& values )
	{
		values.clear();
		
		
		ca::SQLiteStatement stmt = prepareOrThrow( "SELECT F.FIELD_ID, FV.VALUE FROM\
								OBJECT OBJ LEFT OUTER JOIN TYPE T ON OBJ.TYPE_ID = T.TYPE_ID\
								LEFT OUTER JOIN FIELD F ON F.TYPE_ID = T.TYPE_ID\
								LEFT OUTER JOIN (SELECT OBJECT_ID, MAX(REVISION) AS LATEST_REVISION, FIELD_ID, \
								VALUE FROM FIELD_VALUE WHERE REVISION <= ? GROUP BY OBJECT_ID, FIELD_ID ) FV\
								ON FV.OBJECT_ID = OBJ.OBJECT_ID AND F.FIELD_ID = FV.FIELD_ID\
								WHERE OBJ.OBJECT_ID = ? GROUP BY FV.FIELD_ID, FV.OBJECT_ID \
								ORDER BY OBJ.OBJECT_ID" );

		
		stmt.bind( 1, revision );
		stmt.bind( 2, objectId );
		ca::SQLiteResult rs = executeQueryOrThrow( stmt );
				
		while( rs.next() )
		{
			ca::StoredFieldValue sfv;
			sfv.fieldId = rs.getUint32( 0 ); //fieldName;
			sfv.value = rs.getString( 1 );
			values.push_back( sfv );
		}
		stmt.finalize();
	}

	void getType( co::uint32 typeId, ca::StoredType& storedType )
	{
		
		ca::SQLiteStatement stmt = prepareOrThrow( "SELECT T.TYPE_ID, T.TYPE_NAME, F.FIELD_ID, F.FIELD_NAME, F.FIELD_TYPE_ID FROM TYPE T OUTER LEFT JOIN FIELD F ON T.TYPE_ID = F.TYPE_ID WHERE T.TYPE_ID = ?" );

		stmt.bind( 1, typeId );

		ca::SQLiteResult rs = executeQueryOrThrow( stmt );
		
		bool first = true;
		StoredField sf;
		while( rs.next() )
		{
			if( first )
			{
				storedType.fields.clear();
				storedType.typeId = typeId;
				storedType.typeName = rs.getString( 1 );
				
				//check if it has fields...
				if( rs.getString( 3 ) == "" )
				{
					break;
				}
				
				first = false;
			}

			sf.fieldName = rs.getString( 3 );
			sf.fieldId = rs.getUint32( 2 );
			sf.typeId = rs.getUint32( 4 );

			storedType.fields.push_back( sf );
		}
		stmt.finalize();
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
	co::uint32 getRootObjectForRevision( co::uint32 revision )
	{
		co::uint32 rootObject = 0;
		ca::SQLiteStatement stmt = prepareOrThrow( "SELECT ROOT_OBJECT_ID FROM SPACE WHERE REVISION >= ? ORDER BY REVISION LIMIT 1" );

		stmt.bind( 1, revision );

		ca::SQLiteResult rs = executeQueryOrThrow( stmt );
		
		if( rs.next() )
		{
			rootObject = rs.getUint32(0);
			stmt.finalize();
		}
		return rootObject;
	}

	co::uint32 getFieldIdByName( const std::string& fieldName, co::uint32 typeId )
	{
		
		ca::SQLiteStatement stmt = prepareOrThrow( "SELECT FIELD_ID FROM FIELD WHERE FIELD_NAME = ? AND TYPE_ID = ?" );
		stmt.bind( 1, fieldName );
		stmt.bind( 2, typeId );

		ca::SQLiteResult rs = executeQueryOrThrow( stmt );

		co::uint32 result = 0;

		if( rs.next() )
		{
			result = rs.getUint32(0);
		}
		stmt.finalize();

		return result;
	}

	void checkGenerateRevision()
	{
		if( !_startedRevision )
		{
			_latestRevision++;
			_currentRevision = _latestRevision;
			_startedRevision = true;
		}
	}

	void checkBeginTransaction()
	{
		if( !_inTransaction )
		{
			CORAL_THROW(ca::IOException, "Attempt to call a store modification routine without calling 'beginChanges' before.");
		}
	}

	void createTables()
	{
		try{
			_db.prepare("BEGIN TRANSACTION").execute();

			_db.prepare( "CREATE TABLE if not exists [TYPE] (\
				[TYPE_ID] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\
				[TYPE_NAME] VARCHAR(128)  NOT NULL,\
				[TYPE_VERSION] INTEGER  NOT NULL,\
				UNIQUE (TYPE_NAME, TYPE_VERSION)\
				);" ).execute();
			
			_db.prepare( "CREATE TABLE if not exists [FIELD] (\
				[FIELD_NAME] VARCHAR(128) NOT NULL,\
				[TYPE_ID] INTEGER  NOT NULL,\
				[FIELD_TYPE_ID] INTEGER NOT NULL,\
				[FIELD_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
				UNIQUE (FIELD_NAME, TYPE_ID),\
				FOREIGN KEY (TYPE_ID) REFERENCES TYPE(TYPE_ID),\
				FOREIGN KEY (FIELD_TYPE_ID) REFERENCES TYPE(TYPE_ID)\
				);" ).execute();

			_db.prepare( "CREATE TABLE if not exists [OBJECT] (\
				[OBJECT_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
				[TYPE_ID] INTEGER  NULL,\
				FOREIGN KEY (TYPE_ID) REFERENCES TYPE(TYPE_ID)\
				);" ).execute();

			_db.prepare( "CREATE TABLE if not exists [FIELD_VALUE] (\
				[FIELD_ID] INTEGER  NOT NULL,\
				[VALUE] TEXT  NULL,\
				[REVISION] INTEGER NOT NULL,\
				[OBJECT_ID] INTEGER NOT NULL,\
				PRIMARY KEY (FIELD_ID, REVISION, OBJECT_ID),\
				FOREIGN KEY (FIELD_ID) REFERENCES FIELD(FIELD_ID),\
				FOREIGN KEY (OBJECT_ID) REFERENCES OBJECT(OBJECT_ID)\
				);" ).execute();

			_db.prepare( "CREATE TABLE if not exists [SPACE] (\
				[ROOT_OBJECT_ID] INTEGER NOT NULL,\
				[REVISION] INTEGER  NOT NULL,\
				[TIME] TEXT NOT NULL,\
				UNIQUE( REVISION ),\
				FOREIGN KEY (ROOT_OBJECT_ID) REFERENCES OBJECT(OBJECT_ID));" ).execute();

			_db.prepare("COMMIT TRANSACTION").execute();
		}
		catch( ca::SQLiteException& e )
		{
			_db.prepare("ROLLBACK TRANSACTION").execute();
			throw ca::IOException( e.what() );
		}
	}

	void fillLatestRevision()
	{
		ca::SQLiteStatement stmt = prepareOrThrow( "SELECT MAX(REVISION), ROOT_OBJECT_ID FROM SPACE GROUP BY ROOT_OBJECT_ID" );
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
		_currentRevision = _latestRevision;

	}

	ca::SQLiteStatement prepareOrThrow( const std::string& sql )
	{
		try
		{
			return _db.prepare( sql );
		}
		catch ( ca::SQLiteException& e )
		{
			CORAL_THROW( ca::IOException, "Unexpected database query exception, create Statement error: " << e.getMessage() );
		}
	}

	ca::SQLiteResult executeQueryOrThrow( ca::SQLiteStatement& stmt )
	{
		try
		{
			return stmt.query();
		}
		catch ( ca::SQLiteException& e )
		{
			CORAL_THROW( ca::IOException, "Unexpected database query exception: " << e.getMessage() );
		}		
				
	}

	void executeOrThrow( ca::SQLiteStatement& stmt )
	{
		try
		{
			stmt.execute();
		}
		catch ( ca::SQLiteException& e )
		{
			CORAL_THROW( ca::IOException, "Unexpected database query exception: " << e.getMessage() );
		}
	}

private:
	std::string _fileName;
	ca::SQLiteConnection _db;
	co::uint32 _currentRevision;
	co::uint32 _latestRevision;
	co::uint32 _rootObjectId;
	bool _firstObject;
	bool _inTransaction;
	bool _startedRevision;
};

CORAL_EXPORT_COMPONENT( SQLiteSpaceStore, SQLiteSpaceStore );

} // namespace ca
