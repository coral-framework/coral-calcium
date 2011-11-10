#ifndef _DB_SPACE_STORE_QUERIES_
#define _DB_SPACE_STORE_QUERIES_

#include <sstream>
using namespace std;

class DBSpaceStoreQueries
{

	public:
	
		static std::string selectTypeIdByName( std::string typeName, int version )
	{
		std::stringstream ss;
		ss << "SELECT TYPE_ID FROM TYPE WHERE TYPE_NAME = '" << typeName << "'  AND TYPE_VERSION = " << version;

		return ss.str();
	}

	static std::string selectFieldIdByName( std::string fieldName, int typeId )
	{
		std::stringstream ss;
		ss << "SELECT FIELD_ID FROM FIELD WHERE FIELD_NAME = '" << fieldName << "' AND TYPE_ID = " << typeId;

		return ss.str();
	}
	
	static std::string selectFieldValues(int objectId, int revision)
	{
		stringstream revisionSS;
		revisionSS << "WHERE REVISION <= " << revision;
		
		return selectFieldValueRevisioned( objectId, revisionSS.str() );
	}

	static std::string selectFieldValuesLatestVersion(int objectId)
	{
		return selectFieldValueRevisioned(objectId, "");
	}

	static std::string selectFieldValueRevisioned(int objectId, std::string revisionStr)
	{
		stringstream sql;
		sql << "SELECT F.FIELD_ID, FV.VALUE FROM\
			OBJECT OBJ LEFT OUTER JOIN TYPE T ON OBJ.TYPE_ID = T.TYPE_ID\
			LEFT OUTER JOIN FIELD F ON F.TYPE_ID = T.TYPE_ID\
			LEFT OUTER JOIN (SELECT OBJECT_ID, MAX(REVISION) AS LATEST_REVISION, FIELD_ID, VALUE FROM FIELD_VALUE " << revisionStr << " GROUP BY OBJECT_ID, FIELD_ID ) FV\
			ON FV.OBJECT_ID = OBJ.OBJECT_ID AND F.FIELD_ID = FV.FIELD_ID\
			WHERE OBJ.OBJECT_ID = " << objectId <<
			"GROUP BY FV.FIELD_ID, FV.OBJECT_ID\
			ORDER BY OBJ.OBJECT_ID";

		return sql.str();
	}

	static std::string createTableType()
	{
		return "CREATE TABLE if not exists [TYPE] (\
				[TYPE_ID] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\
				[TYPE_NAME] VARCHAR(128)  NOT NULL,\
				[TYPE_VERSION] INTEGER  NOT NULL,\
				UNIQUE (TYPE_NAME, TYPE_VERSION)\
				);";
	}
	
	static std::string createTableField()
	{	
		return "CREATE TABLE if not exists [FIELD] (\
				[FIELD_NAME] VARCHAR(128) NOT NULL,\
				[TYPE_ID] INTEGER  NOT NULL,\
				[FIELD_TYPE_ID] INTEGER NOT NULL,\
				[FIELD_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
				UNIQUE (FIELD_NAME, TYPE_ID),\
				FOREIGN KEY (TYPE_ID) REFERENCES TYPE(TYPE_ID)\
				FOREIGN KEY (FIELD_TYPE_ID) REFERENCES TYPE(TYPE_ID)\
				);";
	}

	static std::string createTableObject()
	{	
		return  "CREATE TABLE if not exists [OBJECT] (\
				[OBJECT_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
				[TYPE_ID] INTEGER  NULL,\
				FOREIGN KEY (TYPE_ID) REFERENCES TYPE(TYPE_ID)\
				);";
	}

	static std::string createTableFieldValues()
	{	
		return "CREATE TABLE if not exists [FIELD_VALUE] (\
				[FIELD_ID] INTEGER  NOT NULL,\
				[VALUE] TEXT  NULL,\
				[REVISION] INTEGER NOT NULL,\
				[OBJECT_ID] INTEGER NOT NULL,\
				PRIMARY KEY (FIELD_ID, REVISION, OBJECT_ID),\
				FOREIGN KEY (FIELD_ID) REFERENCES FIELD(FIELD_ID),\
				FOREIGN KEY (OBJECT_ID) REFERENCES OBJECT(OBJECT_ID)\
				);";
	}

	static std::string insertField(int typeId, std::string fieldName, int fieldTypeId)
	{
		stringstream ss;
		ss << "INSERT INTO FIELD (TYPE_ID, FIELD_NAME, FIELD_TYPE_ID) VALUES ('" << typeId << "', " << fieldName << ", '" << fieldTypeId << "');";
				
		return ss.str();
	}
	
	static std::string insertType( std::string typeName, int version )
	{
		stringstream ss;
		ss << "INSERT INTO TYPE (TYPE_NAME, TYPE_VERSION) VALUES ('" << typeName << "', " << version << ")";
				
		return ss.str();
	}

	static std::string insertFieldValue( int fieldId, int objId, int revision, std::string value )
	{

		stringstream ss;
		ss << "INSERT INTO FIELD_VALUE (FIELD_ID, OBJECT_ID, REVISION, VALUE) VALUES (" << fieldId
							<< ", " << objId << ", " << revision << ", '" << value << "')";
		return ss.str();				
	}

	static std::string insertObject( int typeId )
	{
		stringstream ss;
		ss << "INSERT INTO OBJECT (TYPE_ID) VALUES (" << typeId << ");";

		return ss.str();
	}

	static std::string selectEntityFromObject( int objId )
	{
		stringstream ss;
		ss << "SELECT O.TYPE_ID FROM OBJECT O WHERE O.OBJECT_ID = " << objId;

		return ss.str();
	}

	
	static std::string selectTypeById( co::int32 typeId )
	{
		stringstream ss;
		ss << "SELECT T.TYPE_ID, T.TYPE_NAME, F.FIELD_ID, F.FIELD_NAME, F.FIELD_TYPE_ID FROM TYPE T OUTER LEFT JOIN FIELD F ON T.TYPE_ID = F.TYPE_ID WHERE T.TYPE_ID = " << typeId;

		return ss.str();
	}
	static std::string selectLastInsertedObject()
	{
		return "SELECT MAX(OBJECT_ID) FROM OBJECT";
	}

};
#endif