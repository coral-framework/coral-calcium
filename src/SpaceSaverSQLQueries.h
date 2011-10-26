#ifndef _SPACE_SAVER_SQL_QUERIES_
#define _SPACE_SAVER_SQL_QUERIES_

#include <sstream>
using namespace std;

class SpaceSaverSQLQueries
{

	//sprintf(buffer, "SELECT ENTITY_ID FROM ENTITY WHERE ENTITY_NAME = '%s' AND CAMODEL_ID = %i", entityName.c_str(), calciumModelId);
	//sprintf(buffer, "SELECT CAMODEL_ID FROM CALCIUM_MODEL WHERE CAMODEL_CONTENT = '%s' AND CAMODEL_VERSION = %i", _model->getName().c_str(), _modelVersion);
	public:
	static std::string selectEntityIdByName( std::string entityName, int calciumModelId )
	{
		std::stringstream ss;
		ss << "SELECT ENTITY_ID FROM ENTITY WHERE ENTITY_NAME = '" << entityName << "'  AND CAMODEL_ID = " << calciumModelId;

		return ss.str();
	}

	static std::string selectFieldIdByName( std::string fieldName, int entityId )
	{
		std::stringstream ss;
		ss << "SELECT FIELD_ID FROM FIELD WHERE FIELD_NAME = '" << fieldName << "' AND ENTITY_ID = " << entityId;

		return ss.str();
	}
	
	static std::string selectFieldValues(int objectId, int caModelId, int fieldValueVersion)
	{
		stringstream sql;
		sql << "SELECT F.FIELD_NAME, FV.VALUE FROM " <<
			"FIELD_VALUES FV LEFT OUTER JOIN OBJECT OBJ ON FV.OBJECT_ID = OBJ.OBJECT_ID "<<
			"LEFT OUTER JOIN ENTITY E ON OBJ.ENTITY_ID = E.ENTITY_ID " <<
			"LEFT OUTER JOIN FIELD F ON F.ENTITY_ID = E.ENTITY_ID AND F.FIELD_ID = FV.FIELD_ID " <<
			"LEFT OUTER JOIN CALCIUM_MODEL CM ON E.CAMODEL_ID = CM.CAMODEL_ID "<<
			"WHERE CM.CAMODEL_ID = " << caModelId << " AND FV.FIELD_VALUE_VERSION = " << fieldValueVersion <<
			" AND OBJ.OBJECT_ID = " << objectId;
		
		return sql.str();
	}

	static std::string createTableCalciumModel()
	{
		return "CREATE TABLE if not exists [CALCIUM_MODEL] (\
				[CAMODEL_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
				[CAMODEL_CONTENT] TEXT  NOT NULL,\
				[CAMODEL_VERSION] INTEGER  NOT NULL,\
				UNIQUE (CAMODEL_CONTENT, CAMODEL_VERSION)\
				);";
	}

	static std::string createTableEntity()
	{
		return "CREATE TABLE if not exists [ENTITY] (\
				[ENTITY_ID] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\
				[ENTITY_NAME] VARCHAR(128)  NOT NULL,\
				[CAMODEL_ID] INTEGER  NOT NULL,\
				UNIQUE (ENTITY_NAME, CAMODEL_ID),\
				FOREIGN KEY (CAMODEL_ID) REFERENCES CALCIUM_MODEL(CAMODEL_ID)\
				);";
	}
	
	static std::string createTableField()
	{	
		return "CREATE TABLE if not exists [FIELD] (\
			[FIELD_NAME] VARCHAR(128) NOT NULL,\
			[ENTITY_ID] INTEGER  NOT NULL,\
			[FIELD_TYPE] VARCHAR(30)  NULL,\
			[FIELD_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
			UNIQUE (FIELD_NAME, ENTITY_ID),\
			FOREIGN KEY (ENTITY_ID) REFERENCES ENTITY(ENTITY_ID)\
			);";
	}

	static std::string createTableObject()
	{	
		return  "CREATE TABLE if not exists [OBJECT](\
				[OBJECT_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
				[ENTITY_ID] INTEGER  NULL,\
				FOREIGN KEY (ENTITY_ID) REFERENCES ENTITY(ENTITY_ID)\
				);";
	}

	static std::string createTableFieldValues()
	{	
		return "CREATE TABLE if not exists [FIELD_VALUES](\
				[FIELD_ID] INTEGER  NOT NULL,\
				[VALUE] TEXT  NULL,\
				[FIELD_VALUE_VERSION] INTEGER NOT NULL,\
				[OBJECT_ID] INTEGER NOT NULL,\
				PRIMARY KEY (FIELD_ID, FIELD_VALUE_VERSION, OBJECT_ID),\
				FOREIGN KEY (FIELD_ID) REFERENCES FIELD(FIELD_ID),\
				FOREIGN KEY (OBJECT_ID) REFERENCES OBJECT(OBJECT_ID)\
				);";
	}

	static std::string insertField(std::string fieldName, int entityId, std::string fieldType)
	{
		stringstream ss;
		ss << "INSERT INTO FIELD (FIELD_NAME, ENTITY_ID, FIELD_TYPE) VALUES ('" << fieldName << "', " << entityId << ", '" << fieldType << "');";
				
		return ss.str();
	}
	
	static std::string insertEntity( std::string entityName, int caModelId )
	{
		stringstream ss;
		ss << "INSERT INTO ENTITY (ENTITY_NAME, CAMODEL_ID) VALUES ('" << entityName << "', " << caModelId << ")";
				
		return ss.str();
	}

	static std::string insertRefFieldValue( int fieldId, int objId, int fieldValueVersion, int refId )
	{

		stringstream ss;
		ss << "INSERT INTO FIELD_VALUES (FIELD_ID, OBJECT_ID, FIELD_VALUE_VERSION, VALUE) VALUES (" << fieldId
							<< ", " << objId << ", " << fieldValueVersion << ", '" << refId << "')";

		return ss.str();						
	}

	static std::string insertFieldValue( int fieldId, int objId, int fieldValueVersion, std::string value )
	{
		stringstream ss;
		ss << "INSERT INTO FIELD_VALUES (FIELD_ID, OBJECT_ID, FIELD_VALUE_VERSION, VALUE) VALUES (" << fieldId
							<< ", " << objId << ", " << fieldValueVersion << ", '" << value << "')";

		return ss.str();						
	}

	static std::string insertObject( int entityId )
	{
		stringstream ss;
		ss << "INSERT INTO OBJECT (ENTITY_ID) VALUES (" << entityId << ");";

		return ss.str();
	}

	static std::string selectEntityFromObject( int objId, int caModelId )
	{
		stringstream ss;
		ss << "SELECT E.ENTITY_NAME, E.ENTITY_ID FROM OBJECT OBJ, ENTITY E WHERE OBJ.ENTITY_ID = E.ENTITY_ID AND OBJ.OBJECT_ID = " << objId << " AND E.CAMODEL_ID = " << caModelId;

		return ss.str();
	}

	static std::string selectFieldValue(std::string& portName, int objId, int entityId )
	{
		stringstream ss;
		ss << "SELECT FV.VALUE FROM FIELD F LEFT OUTER JOIN FIELD_VALUES FV ON F.FIELD_ID = FV.FIELD_ID WHERE F.FIELD_NAME = '"
						<< portName <<"' AND FV.OBJECT_ID = " << objId << " AND F.ENTITY_ID = " << entityId;

		return ss.str();
	}

	static std::string selectCalciumModel( std::string& content, int modelVersion )
	{
		stringstream ss;
		ss << "SELECT CAMODEL_ID FROM CALCIUM_MODEL WHERE CAMODEL_CONTENT = '" << content << "' AND CAMODEL_VERSION = " << modelVersion;

		return ss.str();
	}

	static std::string insertCalciumModel( std::string& content, int modelVersion )
	{
		stringstream ss;
		ss << "INSERT INTO CALCIUM_MODEL (CAMODEL_CONTENT, CAMODEL_VERSION) VALUES ('" << content << "', " << modelVersion << ");";

		return ss.str();
	}

};
#endif