#include "SQLiteSpaceStore_Base.h"

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

class SQLiteSpaceStore : public SQLiteSpaceStore_Base
{

public:
	SQLiteSpaceStore()
	{
	}

	virtual ~SQLiteSpaceStore()
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
		catch( ca::DBException& )
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
		ca::SQLitePreparedStatement stmt;
		createStatementOrThrow( "INSERT INTO SPACE VALUES (?, ?, datetime('now') )", stmt );

		stmt.setInt( 1, _rootObjectId );
		stmt.setInt( 2, _currentRevision );

		executeOrThrow( stmt );
		stmt.finalize();
	}

	co::uint32 getRootObject( co::uint32 revision )
	{
		return getRootObjectForRevision( revision );
	}

	co::uint32 getOrAddType( const std::string& typeName, co::uint32 version ) 
	{
		ca::SQLiteResultSet rs; 
		ca::SQLitePreparedStatement typeStmt;

		std::string typeQuery = "SELECT TYPE_ID FROM TYPE WHERE TYPE_NAME = ?  AND TYPE_VERSION = ?";
		createStatementOrThrow( typeQuery, typeStmt );

		typeStmt.setString( 1, typeName.c_str() );
		typeStmt.setInt( 2, version );

		executeQueryOrThrow( typeStmt, rs );

		if( !rs.next() )
		{
			typeStmt.reset();
			rs.finalize();

			ca::SQLitePreparedStatement stmt;
			
			createStatementOrThrow( "INSERT INTO TYPE (TYPE_NAME, TYPE_VERSION) VALUES (?, ?)", stmt );
			stmt.setString( 1, typeName.c_str() );
			stmt.setInt( 2, version );
			
			executeOrThrow( stmt );
			stmt.finalize();
			
			executeQueryOrThrow( typeStmt, rs );
			rs.next();
		}

		int id = atoi( rs.getValue(0).c_str() );
		
		typeStmt.finalize();
		
		return id;

	}
		
	co::uint32 addField( co::uint32 typeId, const std::string& fieldName, co::uint32 fieldTypeId )
	{
		ca::SQLitePreparedStatement stmt;

		createStatementOrThrow( "INSERT INTO FIELD (TYPE_ID, FIELD_NAME, FIELD_TYPE_ID) VALUES ( ?, ?, ? );", stmt );
		
		stmt.setInt( 1, typeId );
		stmt.setString( 2, fieldName.c_str() );
		stmt.setInt( 3, fieldTypeId );

		executeOrThrow( stmt );
		stmt.finalize();

		return getFieldIdByName(fieldName, typeId);
	}
	
	co::uint32 addObject( co::uint32 typeId )
	{

		ca::SQLitePreparedStatement stmt;

		createStatementOrThrow( "INSERT INTO OBJECT (TYPE_ID) VALUES ( ? );", stmt );
		stmt.setInt( 1, typeId );
		executeOrThrow( stmt );
		
		ca::SQLiteResultSet rs; 
		executeQueryOrThrow( "SELECT MAX(OBJECT_ID) FROM OBJECT", rs );

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
		stmt.finalize();
		return resultId;
	}

	void addValues( co::uint32 objId, co::Range<const ca::StoredFieldValue> values )
	{
		ca::SQLitePreparedStatement stmt;

		createStatementOrThrow( "INSERT INTO FIELD_VALUE (FIELD_ID, OBJECT_ID, REVISION, VALUE)\
										 VALUES (?, ?, ?, ?)", stmt );

		for( int i = 0; i < values.getSize(); i++ )
		{
			stmt.reset();
			stmt.setInt( 1, values[i].fieldId );
			stmt.setInt( 2, objId );
			stmt.setInt( 3, _currentRevision );
			stmt.setString( 4, values[i].value.c_str() );

			executeOrThrow( stmt );
		}
		stmt.finalize();
	}
	
	co::uint32 getObjectType( co::uint32 objectId )
	{
		ca::SQLitePreparedStatement stmt;
		ca::SQLiteResultSet rs; 
		
		createStatementOrThrow( "SELECT O.TYPE_ID FROM OBJECT O WHERE O.OBJECT_ID = ?", stmt );

		stmt.setInt( 1, objectId );

		executeQueryOrThrow( stmt, rs );
				
		if(rs.next())
		{
			int id = atoi(rs.getValue(0).c_str());
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
		ca::SQLitePreparedStatement stmt;
		ca::SQLiteResultSet rs;
		
		createStatementOrThrow( "SELECT F.FIELD_ID, FV.VALUE FROM\
								OBJECT OBJ LEFT OUTER JOIN TYPE T ON OBJ.TYPE_ID = T.TYPE_ID\
								LEFT OUTER JOIN FIELD F ON F.TYPE_ID = T.TYPE_ID\
								LEFT OUTER JOIN (SELECT OBJECT_ID, MAX(REVISION) AS LATEST_REVISION, FIELD_ID, \
								VALUE FROM FIELD_VALUE WHERE REVISION <= ? GROUP BY OBJECT_ID, FIELD_ID ) FV\
								ON FV.OBJECT_ID = OBJ.OBJECT_ID AND F.FIELD_ID = FV.FIELD_ID\
								WHERE OBJ.OBJECT_ID = ? GROUP BY FV.FIELD_ID, FV.OBJECT_ID \
								ORDER BY OBJ.OBJECT_ID", stmt );

		
		stmt.setInt( 1, revision );
		stmt.setInt( 2, objectId );
		executeQueryOrThrow( stmt, rs );
				
		while( rs.next() )
		{
			ca::StoredFieldValue sfv;
			sfv.fieldId = atoi( rs.getValue( 0 ).c_str() ); //fieldName;
			sfv.value = rs.getValue( 1 );
			values.push_back( sfv );
		}
		stmt.finalize();
	}

	void getType( co::uint32 typeId, ca::StoredType& storedType )
	{
		ca::SQLitePreparedStatement stmt;
		ca::SQLiteResultSet rs;
		createStatementOrThrow( "SELECT T.TYPE_ID, T.TYPE_NAME, F.FIELD_ID, F.FIELD_NAME, F.FIELD_TYPE_ID FROM TYPE T OUTER LEFT JOIN FIELD F ON T.TYPE_ID = F.TYPE_ID WHERE T.TYPE_ID = ?", stmt );

		stmt.setInt( 1, typeId );

		executeQueryOrThrow( stmt, rs );
		
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

	std::string _fileName;
	ca::SQLiteConnection _db;
	co::uint32 _currentRevision;
	co::uint32 _latestRevision;
	co::uint32 _rootObjectId;
	bool _firstObject;
		
	co::uint32 getRootObjectForRevision( co::uint32 revision )
	{
		co::uint32 rootObject = 0;
		ca::SQLitePreparedStatement stmt;
		createStatementOrThrow( "SELECT ROOT_OBJECT_ID FROM SPACE WHERE REVISION >= ? ORDER BY REVISION LIMIT 1", stmt );

		stmt.setInt( 1, revision );

		ca::SQLiteResultSet rs;
		executeQueryOrThrow( stmt, rs );
		
		if( rs.next() )
		{
			rootObject = atoi( rs.getValue(0).c_str() );
			stmt.finalize();
		}
		return rootObject;
	}

	co::uint32 getFieldIdByName( const std::string& fieldName, co::uint32 typeId )
	{
		ca::SQLitePreparedStatement stmt;
		ca::SQLiteResultSet rs;

		createStatementOrThrow( "SELECT FIELD_ID FROM FIELD WHERE FIELD_NAME = ? AND TYPE_ID = ?", stmt );
		stmt.setString( 1, fieldName.c_str() );
		stmt.setInt( 2, typeId );

		executeQueryOrThrow( stmt, rs );

		int result = -1;

		if( rs.next() )
		{
			result = atoi( rs.getValue(0).c_str() );
		}
		stmt.finalize();

		return result;
	}

	void createTables()
	{
		try{
			_db.execute("BEGIN TRANSACTION");

			_db.execute("PRAGMA foreign_keys = 1");

			_db.execute( "CREATE TABLE if not exists [TYPE] (\
				[TYPE_ID] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\
				[TYPE_NAME] VARCHAR(128)  NOT NULL,\
				[TYPE_VERSION] INTEGER  NOT NULL,\
				UNIQUE (TYPE_NAME, TYPE_VERSION)\
				);" );
			
			_db.execute( "CREATE TABLE if not exists [FIELD] (\
				[FIELD_NAME] VARCHAR(128) NOT NULL,\
				[TYPE_ID] INTEGER  NOT NULL,\
				[FIELD_TYPE_ID] INTEGER NOT NULL,\
				[FIELD_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
				UNIQUE (FIELD_NAME, TYPE_ID),\
				FOREIGN KEY (TYPE_ID) REFERENCES TYPE(TYPE_ID)\
				FOREIGN KEY (FIELD_TYPE_ID) REFERENCES TYPE(TYPE_ID)\
				);" );

			_db.execute( "CREATE TABLE if not exists [OBJECT] (\
				[OBJECT_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
				[TYPE_ID] INTEGER  NULL,\
				FOREIGN KEY (TYPE_ID) REFERENCES TYPE(TYPE_ID)\
				);" );

			_db.execute( "CREATE TABLE if not exists [FIELD_VALUE] (\
				[FIELD_ID] INTEGER  NOT NULL,\
				[VALUE] TEXT  NULL,\
				[REVISION] INTEGER NOT NULL,\
				[OBJECT_ID] INTEGER NOT NULL,\
				PRIMARY KEY (FIELD_ID, REVISION, OBJECT_ID),\
				FOREIGN KEY (FIELD_ID) REFERENCES FIELD(FIELD_ID),\
				FOREIGN KEY (OBJECT_ID) REFERENCES OBJECT(OBJECT_ID)\
				);" );

			_db.execute( "CREATE TABLE if not exists [SPACE] (\
				[ROOT_OBJECT_ID] INTEGER NOT NULL,\
				[REVISION] INTEGER  NOT NULL,\
				[TIME] TEXT NOT NULL,\
				UNIQUE( REVISION ),\
				FOREIGN KEY (ROOT_OBJECT_ID) REFERENCES OBJECT(OBJECT_ID));" );

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
		executeQueryOrThrow( "SELECT MAX(REVISION), ROOT_OBJECT_ID FROM SPACE GROUP BY ROOT_OBJECT_ID", rs );
			
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

	void createStatementOrThrow( const std::string& sql, ca::SQLitePreparedStatement& stmt )
	{
		try
		{
			_db.createPreparedStatement( sql, stmt );
		}
		catch ( ca::DBException& )
		{
			throw ca::IOException( "Unexpected database query exception: Create Statement error" );
		}
	}

	void executeQueryOrThrow( ca::SQLitePreparedStatement& stmt, ca::SQLiteResultSet& rs )
	{
		try
		{
			stmt.execute( rs );
		}
		catch ( ca::DBException& )
		{
			throw ca::IOException( "Unexpected database query exception" );
		}		
				
	}

	void executeOrThrow( ca::SQLitePreparedStatement& stmt )
	{
		try
		{
			stmt.execute();
		}
		catch ( ca::DBException& )
		{
			throw ca::IOException( "Unexpected database query exception" );
		}		
				
	}

	void executeQueryOrThrow( const std::string& sql, ca::SQLiteResultSet& rs )
	{
		try
		{
			_db.executeQuery( sql, rs );
		}
		catch ( ca::DBException& )
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
		catch ( ca::DBException& )
		{
			throw ca::IOException( "Unexpected database query exception" );
		}	
	}

};
CORAL_EXPORT_COMPONENT( SQLiteSpaceStore, SQLiteSpaceStore );

} // namespace ca