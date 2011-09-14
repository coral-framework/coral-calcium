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
#include <co/IComponent.h>
#include <co/IPort.h>
#include <stdio.h>
#include <string.h>


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
				
				sqlite3_open( "space.db", &db );
				sqlite3_exec( db, "PRAGMA foreign_keys = 1", callback, 0, &error );
				if( error )
					puts( error );

				create_tables();

				createModelMetadata();

				co::RefPtr<co::IObject> object;
				co::RefVector<co::IPort>  ports;
				co::RefVector<co::IField> fields;
				char buffer[100];

				object = _space->getRootObject();

				sprintf( buffer, "insert object values (%d)");
				sqlite3_exec( db, buffer, callback, 0, &error );
				if( error )
					puts( error );
				_model->getPorts( object->getComponent(), ports);
				for( co::Range<co::IPort*> r( ports ); r; r.popFirst() )
				{
					// for each port, adds a row in ports table 
					const char* name = r.getFirst()->getname().c_str();
					_model->getFields( r.getFirst()->getType(), fields );
					//sprintf( buffer, "insert into ports( object_id, name ) values( %d, '%s' )", i, name );
					sqlite3_exec( db, buffer, callback, 0, &error );
					if( error )
						puts( error );
					for( int j = 0; j < fields.size(); j++)
					{
						printf("%s\n", fields[j]->getOwner()->getMembers()[fields[j]->getIndex()]->getName().c_str() );
						printf("%s\n", fields[j]->getName().c_str() );
						printf("%s\n", fields[j]->getType()->getName().c_str() );
					}
				}				
					
				sqlite3_free( error );
			}

			void setModel( ca::IModel* model )
			{
				_model = model;
			}

			void setSpace( ca::ISpace* space )
			{
				_space = space;
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
			co::RefPtr<cca::ISpaceChanges> spaceChanges; 
			// member variables go here
			// A callback to show the result of a query. It just prints for test reasons
			static int callback(void *NotUsed, int argc, char **argv, char **azColName)
			{
				int i;
				for(i=0; i<argc; i++){
					printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
				}
				printf("\n");
				return 0;
			}

			void createModelMetadata()
			{

			}

			

			void create_tables()
			{
				char *error;
				
				sqlite3_open( "space.db", &db );
				sqlite3_exec( db, "PRAGMA foreign_keys = 1", callback, 0, &error );
				if( error )
					puts( error );
				sqlite3_free( error );

				sqlite3_exec(db, "BEGIN", 0, 0, 0);

				char buffer[500];

				sprintf( buffer, "CREATE TABLE [CALCIUM_MODEL] if not exists (\
								  [CAMODEL_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
								  [CAMODEL_CONTENT] TEXT  NOT NULL,\
								  [CAMODEL_VERSION] INTEGER  NOT NULL\
								  );" );
				sqlite3_exec( db, buffer, callback, 0, &error );
				if(error)
					puts(error);
				

				sprintf( buffer, "CREATE TABLE [ENTITY] if not exists (\
								 [ENTITY_ID] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,\
								 [ENTITY_NAME] VARCHAR(128)  NOT NULL,\
								 [CAMODEL_ID] INTEGER  NOT NULL,\
								 UNIQUE (ENTITY_NAME, CAMODEL_ID),\
								 FOREIGN KEY (CAMODEL_ID) REFERENCES CALCIUM_MODEL(CAMODEL_ID)\
								 );" );
				sqlite3_exec( db, buffer, callback, 0, &error );
				if(error)
					puts(error);
				
				sprintf( buffer, "CREATE TABLE [FIELD] if not exists (\
								  [FIELD_NAME] VARCHAR(128) NOT NULL,\
								  [ENTITY_ID] INTEGER  NOT NULL,\
								  [FIELD_TYPE] VARCHAR(30)  NULL,\
								  [FIELD_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
								  UNIQUE (FIELD_NAME, ENTITY_ID),\
								  FOREIGN KEY (ENTITY_ID) REFERENCES ENTITY(ENTITY_ID)\
								  );" );
				sqlite3_exec( db, buffer, callback, 0, &error );
				if(error)
					puts(error);

				sprintf( buffer, "CREATE TABLE [OBJECT] if not exists (\
								 [OBJECT_ID] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,\
								 [ENTITY_ID] INTEGER  NULL,\
								 FOREIGN KEY (ENTITY_ID) REFERENCES ENTITY(ENTITY_ID)\
								 );");

				sqlite3_exec( db, buffer, callback, 0, &error );
				if(error)
					puts(error);

				sprintf( buffer, "CREATE TABLE [FIELD_VALUES] if not exists (\
								 [FIELD_ID] INTEGER  NOT NULL,\
								 [VALUE] TEXT  NULL,\
								 [FIELD_VALUE_VERSION] INTEGER NOT NULL,\
								 [OBJECT_ID] INTEGER NOT NULL,\
								 PRIMARY KEY (FIELD_ID, FIELD_VALUE_VERSION, OBJECT_ID),\
								 FOREIGN KEY (FIELD_ID) REFERENCES FIELD(FIELD_ID),\
								 FOREIGN KEY (OBJECT_ID) REFERENCES OBJECT(OBJECT_ID)\
								 );");
				sqlite3_exec( db, buffer, callback, 0, &error );
				if(error)
					puts(error);
				
				sqlite3_exec(db, "COMMIT", 0, 0, 0);

				sqlite3_free( error );
			}
	};

	CORAL_EXPORT_COMPONENT( SpaceSaverSQLite3, SpaceSaverSQLite3 );

} // namespace ca
