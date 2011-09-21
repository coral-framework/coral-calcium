#include "SpaceSaverSQLite3_Base.h"
#include <ca/ISpaceChanges.h>
#include <ca/IUniverse.h>
#include <ca/ISpace.h>
#include <ca/IModel.h>
#include <ca/IDBConnection.h>
#include <ca/IResultSet.h>
#include <ca/DBException.h>
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
				// empty destructor
			}

			// ------ ca.ISpaceObserver Methods ------ //

			void onSpaceChanged( ca::ISpaceChanges* changes )
			{
				spaceChanges.push_back( changes );
			}

			void setup()
			{
				co::RefPtr<co::IObject> db = co::newInstance("ca.SQLiteDBConnection");

				_db = db->getService<ca::IDBConnection>();				
				_db->getProvider()->getService<ca::INamed>()->setName(_fileName);


				_db->createDatabase();

				createTables();

				createModelMetadata();

				_db->close();

			}

			void setSpace( ca::ISpace* space )
			{
				_space = space;
				ca::IUniverse* universe = static_cast<ca::IUniverse*>(_space->getProvider()->getService("universe"));

				assert(universe);
				_model = static_cast<ca::IModel*>( universe->getProvider()->getService("model"));
				assert(_model);

				_modelVersion = _model->getVersion();
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
			co::RefPtr<ca::ISpace> _space;
			co::RefPtr<ca::IModel> _model;
			co::RefPtr<ca::IDBConnection> _db;
			co::int32 _modelVersion;

			std::string _fileName;

			co::RefVector<ca::ISpaceChanges> spaceChanges; 

			char *error;
			char buffer[500];

			int getEntityId(std::string entityName, int calciumModelId)
			{
				sprintf(buffer, "SELECT ENTITY_ID FROM ENTITY WHERE ENTITY_NAME = '%s' AND CAMODEL_ID = %i", entityName.c_str(), calciumModelId);

				co::RefPtr<ca::IResultSet> rs = _db->executeQuery(buffer);
				if(rs->next())
				{
					return atoi(rs->getValue(0).c_str());
				}
				
				return -1;
			}

			int getCalciumModelId()
			{
				sprintf(buffer, "SELECT CAMODEL_ID FROM CALCIUM_MODEL WHERE CAMODEL_CONTENT = '%s' AND CAMODEL_VERSION = %i", _model->getName().c_str(), _modelVersion);
				co::RefPtr<ca::IResultSet> rs = _db->executeQuery(buffer);
				if(rs->next())
				{
					return atoi(rs->getValue(0).c_str());
				}
				return -1;
			}

			int getFieldId(std::string fieldName, int entityId)
			{
				sprintf(buffer, "SELECT FIELD_ID FROM FIELD WHERE FIELD_NAME = '%s' AND ENTITY_ID = %i", fieldName.c_str(), entityId);
				co::RefPtr<ca::IResultSet> rs = _db->executeQuery(buffer);
				if(rs->next())
				{
					return atoi(rs->getValue(0).c_str());
				}
				return -1;
			}


			void createModelMetadata()
			{
				
				co::IComponent* component = _space->getRootObject()->getComponent();

				saveEntity(component);
			}

			void saveComponent(co::IComponent* component)
			{
				
				std::string compName = component->getFullName();
				sprintf(buffer, "INSERT INTO ENTITY (ENTITY_NAME, CAMODEL_ID) VALUES ('%s', %i)", compName.c_str(), getCalciumModelId());
				_db->execute(buffer);

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
				
				_db->execute(buffer);
			}

			void saveInterface(co::IInterface* entityInterface)
			{

				sprintf(buffer, "INSERT INTO ENTITY (ENTITY_NAME, CAMODEL_ID) VALUES ('%s', %i)", entityInterface->getFullName().c_str(), getCalciumModelId());
				_db->execute(buffer);

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
				
				try{
					_db->execute("BEGIN TRANSACTION");

					_db->execute("PRAGMA foreign_keys = 1");

					sprintf( buffer, "CREATE TABLE if not exists [CALCIUM_MODEL] (\
									  [CAMODEL_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
									  [CAMODEL_CONTENT] TEXT  NOT NULL,\
									  [CAMODEL_VERSION] INTEGER  NOT NULL,\
									  UNIQUE (CAMODEL_CONTENT, CAMODEL_VERSION)\
									  );" );
					_db->execute(buffer);

					sprintf( buffer, "CREATE TABLE if not exists [ENTITY] (\
									 [ENTITY_ID] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\
									 [ENTITY_NAME] VARCHAR(128)  NOT NULL,\
									 [CAMODEL_ID] INTEGER  NOT NULL,\
									 UNIQUE (ENTITY_NAME, CAMODEL_ID),\
									 FOREIGN KEY (CAMODEL_ID) REFERENCES CALCIUM_MODEL(CAMODEL_ID)\
									 );" );
					_db->execute(buffer);
				
					sprintf( buffer, "CREATE TABLE if not exists [FIELD] (\
									  [FIELD_NAME] VARCHAR(128) NOT NULL,\
									  [ENTITY_ID] INTEGER  NOT NULL,\
									  [FIELD_TYPE] VARCHAR(30)  NULL,\
									  [FIELD_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
									  UNIQUE (FIELD_NAME, ENTITY_ID),\
									  FOREIGN KEY (ENTITY_ID) REFERENCES ENTITY(ENTITY_ID)\
									  );" );
					_db->execute(buffer);

					sprintf( buffer, "CREATE TABLE if not exists [OBJECT](\
									 [OBJECT_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
									 [ENTITY_ID] INTEGER  NULL,\
									 FOREIGN KEY (ENTITY_ID) REFERENCES ENTITY(ENTITY_ID)\
									 );");

					_db->execute(buffer);
					sprintf( buffer, "CREATE TABLE if not exists [FIELD_VALUES](\
									 [FIELD_ID] INTEGER  NOT NULL,\
									 [VALUE] TEXT  NULL,\
									 [FIELD_VALUE_VERSION] INTEGER NOT NULL,\
									 [OBJECT_ID] INTEGER NOT NULL,\
									 PRIMARY KEY (FIELD_ID, FIELD_VALUE_VERSION, OBJECT_ID),\
									 FOREIGN KEY (FIELD_ID) REFERENCES FIELD(FIELD_ID),\
									 FOREIGN KEY (OBJECT_ID) REFERENCES OBJECT(OBJECT_ID)\
									 );");

					_db->execute(buffer);
				
					sprintf( buffer, "INSERT INTO CALCIUM_MODEL (CAMODEL_CONTENT, CAMODEL_VERSION) VALUES ('%s', %i)", _model->getName().c_str(), _modelVersion);
					_db->execute(buffer);

					_db->execute("COMMIT TRANSACTION");
				}
				catch(ca::DBException e)
				{
					_db->execute("ROLLBACK TRANSACTION");
					throw e;
				}

			}
	};

	CORAL_EXPORT_COMPONENT( SpaceSaverSQLite3, SpaceSaverSQLite3 );
	

} // namespace ca
