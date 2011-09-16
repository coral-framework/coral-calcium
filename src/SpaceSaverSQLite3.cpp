#include "SpaceSaverSQLite3_Base.h"
#include <ca/ISpaceChanges.h>
#include <ca/ISpace.h>
#include <ca/IModel.h>
#include "sqlite3.h"
#include <co/RefVector.h>
#include <co/RefPtr.h>
#include <co/IObject.h>
#include <co/IField.h>
#include <co/Coral.h>
#include <co/IArray.h>
#include <co/IComponent.h>
#include <co/IPort.h>
#include <stdio.h>
#include <string.h>
#include <map>
using namespace std;


namespace ca {

	class SpaceSaverSQLite3 : public SpaceSaverSQLite3_Base
	{
		public:
			SpaceSaverSQLite3()
			{
			}

			virtual ~SpaceSaverSQLite3()
			{
				
				sqlite3_close( db );
				// empty destructor
			}

			// ------ ca.ISpaceObserver Methods ------ //

			void onSpaceChanged( ca::ISpaceChanges* changes )
			{
				spaceChanges.push_back( changes );
			}

			void setup()
			{
				char *error;
				
				sqlite3_open( _fileName.c_str(), &db );
				sqlite3_exec( db, "PRAGMA foreign_keys = 1", 0, 0, &error );
				if( error )
					puts( error );

				createTables();

				createModelMetadata();

				sqlite3_free( error );
			}

			void setModel( ca::IModel* model )
			{
				_model = model;
			}

			void setModelVersion( co::int32 modelVersion)
			{
				_modelVersion = modelVersion;
			}

			void setSpace( ca::ISpace* space )
			{
				_space = space;
			}

			void setName(const std::string& name)
			{
				_fileName = name;
			}

			const std::string& getName()
			{
				return _fileName;
			}

			// ------ ca.ISpaceSaver Methods ------ //

			ca::ISpace* getLatestVersion()
			{
				// TODO: implement this method.
				return NULL;
			}

			ca::ISpace* getVersion( co::int32 version )
			{
				// TODO: implement this method.
				return NULL;
			}

			void saveChanges()
			{
				// TODO: implement this method.
			}

		private:
			sqlite3* db;
			co::RefPtr<ca::ISpace> _space;
			co::RefPtr<ca::IModel> _model;
			co::int32 _modelVersion;

			std::string _fileName;

			co::RefVector<ca::ISpaceChanges> spaceChanges; 

			char *error;
			char buffer[500];

			static int queriedId;

			static int entityIdCallback(void *NotUsed, int argc, char **argv, char **azColName)
			{
				if(argv)
					queriedId = argv[0] ? atoi(argv[0]) : -1;
				return 0;
			}

			int getEntityId(std::string entityName, int calciumModelId)
			{
				sprintf(buffer, "SELECT ENTITY_ID FROM ENTITY WHERE ENTITY_NAME = '%s' AND CAMODEL_ID = %i", entityName.c_str(), calciumModelId);

				queriedId = -1;
				sqlite3_exec( db, buffer, entityIdCallback, 0, &error );
				return queriedId;
			}

			static int calciumModelIdCallback(void *NotUsed, int argc, char **argv, char **azColName)
			{
				if(argv)
					queriedId = argv[0] ? atoi(argv[0]) : -1;
				return 0;
			}

			int getCalciumModelId()
			{
				sprintf(buffer, "SELECT CAMODEL_ID FROM CALCIUM_MODEL WHERE CAMODEL_CONTENT = '%s' AND CAMODEL_VERSION = %i", _model->getName().c_str(), _modelVersion);
				queriedId = -1;
				sqlite3_exec( db, buffer, calciumModelIdCallback, 0, &error );
				return queriedId;
			}

			static int fieldIdCallback(void *NotUsed, int argc, char **argv, char **azColName)
			{
				if(argv)
					queriedId = argv[0] ? atoi(argv[0]) : -1;
				return 0;
			}
						
			int getFieldId(std::string fieldName, int entityId)
			{
				sprintf(buffer, "SELECT FIELD_ID FROM FIELD WHERE FIELD_NAME = '%s' AND ENTITY_ID = %i", fieldName.c_str(), entityId);
				queriedId = -1;
				sqlite3_exec( db, buffer, fieldIdCallback, 0, &error );
				return queriedId;
			}


			// member variables go here
			// A callback to show the result of a query. It just prints for test reasons

			void createModelMetadata()
			{
				
				co::IComponent* component = _space->getRootObject()->getComponent();

				saveEntity(component);
			}

			void saveComponent(co::IComponent* component)
			{
				
				std::string compName = component->getFullName();
				sprintf(buffer, "INSERT INTO ENTITY (ENTITY_NAME, CAMODEL_ID) VALUES ('%s', %i)", compName.c_str(), getCalciumModelId());
				sqlite3_exec( db, buffer, 0, 0, &error );

				co::RefVector<co::IPort> ports;
				_model->getPorts(component, ports);
				
				for(co::RefVector<co::IPort>::iterator it = ports.begin(); it != ports.end(); it++) 
				{
					(*it).get()->getFacet();
					
					savePort(component, (*it).get());
				}

			}
			
			bool entityAlreadyInserted(co::IType* entityInterface)
			{
				return (getEntityId(entityInterface->getFullName(), getCalciumModelId()) != -1);
			}


			void savePort(co::IComponent* component, co::IPort* port)
			{
				
				int componentId = getEntityId(component->getFullName(), getCalciumModelId()); // estrutura de dados pra armazenar entities já inseridas

				saveField(port->getName(), componentId, port->getType()->getFullName());
				
				saveEntity(port->getType());
			}

			bool needsToSaveType(co::IType* type)
			{
				if(type->getKind() != co::TK_COMPONENT &&
				   type->getKind() != co::TK_INTERFACE &&
				   type->getKind() != co::TK_NATIVECLASS)
				{
					return false; //primitive type
				}

				return !entityAlreadyInserted(type);
			}

			void saveEntity(co::IType* type)
			{
				if( !needsToSaveType(type) )
				{
					return;
				}

				if( type->getKind() == co::TK_COMPONENT )
				{
					co::IComponent*	component = static_cast<co::IComponent*>(type);
					saveComponent(component);
					return;
				}

				if( type->getKind() == co::TK_INTERFACE )
				{
					co::IInterface*	interfaceType = static_cast<co::IInterface*>(type);
					saveInterface(interfaceType);
					return;
				}
			}

			

			void saveField(std::string fieldName, int entityId, std::string fieldType)
			{
				sprintf(buffer, "INSERT INTO FIELD (FIELD_NAME, ENTITY_ID, FIELD_TYPE) VALUES ('%s', %i, '%s');", fieldName.c_str(), entityId, fieldType.c_str());
				
				sqlite3_exec( db, buffer, 0, 0, &error );
			}

			void saveInterface(co::IInterface* entityInterface)
			{

				sprintf(buffer, "INSERT INTO ENTITY (ENTITY_NAME, CAMODEL_ID) VALUES ('%s', %i)", entityInterface->getFullName().c_str(), getCalciumModelId());
				sqlite3_exec( db, buffer, 0, 0, &error );

				int getIdFromSavedEntity = getEntityId(entityInterface->getFullName(), getCalciumModelId());

				co::RefVector<co::IField> fields;
				_model->getFields(entityInterface, fields);

				for(co::RefVector<co::IField>::iterator it = fields.begin(); it != fields.end(); it++) 
				{
					co::IField* field =  it->get();
					std::string debugStr = field->getType()->getFullName();
					if(field->getType()->getKind() == co::TK_ARRAY)
					{
						co::IType* arrayType = static_cast<co::IArray*>(field->getType())->getElementType();
						
						saveEntity(arrayType);
					}
					if(field->getType()->getKind() == co::TK_INTERFACE || field->getType()->getKind() == co::TK_NATIVECLASS)
					{
						saveEntity(field->getType());
					}
					
					saveField(field->getName(), getIdFromSavedEntity, field->getType()->getFullName());
				}

			}
			

			void createTables()
			{
				
				sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);

				sprintf( buffer, "CREATE TABLE if not exists [CALCIUM_MODEL] (\
								  [CAMODEL_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
								  [CAMODEL_CONTENT] TEXT  NOT NULL,\
								  [CAMODEL_VERSION] INTEGER  NOT NULL,\
								  UNIQUE (CAMODEL_CONTENT, CAMODEL_VERSION)\
								  );" );
				sqlite3_exec( db, buffer, 0, 0, &error );
				if(error)
				{
					sqlite3_exec(db, "ROLLBACK TRANSACTION", 0, 0, &error);
				}
				

				sprintf( buffer, "CREATE TABLE if not exists [ENTITY] (\
								 [ENTITY_ID] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\
								 [ENTITY_NAME] VARCHAR(128)  NOT NULL,\
								 [CAMODEL_ID] INTEGER  NOT NULL,\
								 UNIQUE (ENTITY_NAME, CAMODEL_ID),\
								 FOREIGN KEY (CAMODEL_ID) REFERENCES CALCIUM_MODEL(CAMODEL_ID)\
								 );" );
				sqlite3_exec( db, buffer, 0, 0, &error );
				if(error)
				{
					sqlite3_exec(db, "ROLLBACK TRANSACTION", 0, 0, &error);
				}
				
				sprintf( buffer, "CREATE TABLE if not exists [FIELD] (\
								  [FIELD_NAME] VARCHAR(128) NOT NULL,\
								  [ENTITY_ID] INTEGER  NOT NULL,\
								  [FIELD_TYPE] VARCHAR(30)  NULL,\
								  [FIELD_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
								  UNIQUE (FIELD_NAME, ENTITY_ID),\
								  FOREIGN KEY (ENTITY_ID) REFERENCES ENTITY(ENTITY_ID)\
								  );" );
				sqlite3_exec( db, buffer, 0, 0, &error );
				if(error)
				{
					sqlite3_exec(db, "ROLLBACK TRANSACTION", 0, 0, &error);
				}

				sprintf( buffer, "CREATE TABLE if not exists [OBJECT](\
								 [OBJECT_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
								 [ENTITY_ID] INTEGER  NULL,\
								 FOREIGN KEY (ENTITY_ID) REFERENCES ENTITY(ENTITY_ID)\
								 );");

				sqlite3_exec( db, buffer, 0, 0, &error );
				if(error)
				{
					sqlite3_exec(db, "ROLLBACK TRANSACTION", 0, 0, &error);
				}

				sprintf( buffer, "CREATE TABLE if not exists [FIELD_VALUES](\
								 [FIELD_ID] INTEGER  NOT NULL,\
								 [VALUE] TEXT  NULL,\
								 [FIELD_VALUE_VERSION] INTEGER NOT NULL,\
								 [OBJECT_ID] INTEGER NOT NULL,\
								 PRIMARY KEY (FIELD_ID, FIELD_VALUE_VERSION, OBJECT_ID),\
								 FOREIGN KEY (FIELD_ID) REFERENCES FIELD(FIELD_ID),\
								 FOREIGN KEY (OBJECT_ID) REFERENCES OBJECT(OBJECT_ID)\
								 );");
				sqlite3_exec( db, buffer, 0, 0, &error );
				if(error)
				{
					sqlite3_exec(db, "ROLLBACK TRANSACTION", 0, 0, &error);
				}
				
				sprintf( buffer, "INSERT INTO CALCIUM_MODEL (CAMODEL_CONTENT, CAMODEL_VERSION) VALUES ('%s', %i)", _model->getName().c_str(), _modelVersion);
				sqlite3_exec( db, buffer, 0, 0, &error );

				sqlite3_exec(db, "COMMIT TRANSACTION", 0, 0, 0);

				sqlite3_free( error );
			}
	};

	int ca::SpaceSaverSQLite3::queriedId = -1;
	CORAL_EXPORT_COMPONENT( SpaceSaverSQLite3, SpaceSaverSQLite3 );
	

} // namespace ca
