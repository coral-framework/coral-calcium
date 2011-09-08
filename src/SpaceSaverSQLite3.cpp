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
				char* error;
				sqlite3_open( "space.db", &db );
				sqlite3_exec( db, "PRAGMA foreign_keys = 1", callback, 0, &error );
				if( error )
					puts( error );
				sqlite3_free( error );
			}

			virtual ~SpaceSaverSQLite3()
			{
				sqlite3_close( db );
				// empty destructor
			}

			// ------ ca.ISpaceObserver Methods ------ //

			void onSpaceChanged( ca::ISpaceChanges* changes )
			{
				// TODO: implement this method.
			}

			void setup()
			{
				create_tables();
				co::RefVector<co::IObject> objects;
				co::RefVector<co::IPort>  ports;
				co::RefVector<co::IField> fields;
				char buffer[100];
				char* error;
				_space->getRootObjects( objects );
				for( int i = 0; i < objects.size(); i++ ) 
				{
					sprintf( buffer, "insert into objects values (%d)", i );
					sqlite3_exec( db, buffer, callback, 0, &error );
					if( error )
						puts( error );
					_model->getPorts( objects[i]->getComponent(), ports);
					for( co::Range<co::IPort*> r( ports ); r; r.popFirst() )
					{
						// For each port, adds a row in ports table 
						const char* name = r.getFirst()->getName().c_str();
						_model->getFields( r.getFirst()->getType(), fields );
						sprintf( buffer, "insert into ports( object_id, name ) values( %d, '%s' )", i, name );
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
			void create_tables()
			{
				char *error;
				char buffer[500];
				sprintf( buffer, "create table if not exists 'objects'\
						( id integer not null primary key autoincrement )" );
				sqlite3_exec( db, buffer, callback, 0, &error );
				if( error )
					puts( error );
				sprintf( buffer, "create table if not exists 'ports'\
						( id integer not null primary key autoincrement, object_id integer not null, name text not null )" );
				sqlite3_exec( db, buffer, callback, 0, &error );
				if(error)
					puts(error);
				sprintf( buffer, "create table if not exists 'fields'\
						( id integer not null, port_id integer not null );");
				sqlite3_exec( db, buffer, callback, 0, &error );
				if(error)
					puts(error);
				sprintf( buffer, "create table if not exists 'field_values'\
						( id integer not null primary key, name text not null );");
				sqlite3_exec( db, buffer, callback, 0, &error );
				if(error)
					puts(error);
				sqlite3_free( error );
			}
	};

	CORAL_EXPORT_COMPONENT( SpaceSaverSQLite3, SpaceSaverSQLite3 );

} // namespace ca
