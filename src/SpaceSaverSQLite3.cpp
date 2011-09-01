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
				co::RefVector<co::IField> fields;
				co::RefVector<co::IObject> objects;
				char buffer[100];
				char* error;
				_space->getRootObjects( objects );
				for( int i = 0; i < objects.size(); i++ ) 
				{
					// For each object, creates its own table.
					sprintf( buffer, "create table if not exists 'object_%d'\
							( id integer not null primary key )", i );
					sqlite3_exec( db, buffer, NULL, 0, NULL );
					for( co::Range<co::IPort* const>  ports = objects[i]->getComponent()->getPorts(); ports; ports.popFirst() )
					{
						// For each port, creates its own table and
						// a collum on the object table
						const char* name = ports.getFirst()->getName().c_str();
						_model->getFields( ports.getFirst()->getType(), fields );
						// Create a table for the port
						sprintf( buffer, "create table if not exists 'object_%d_port_%s'\
								( id integer not null primary key );", i, name );
						sqlite3_exec( db, buffer, NULL, 0, &error );
						if(error)
							puts(error);
						// if there isnt a column on the object referencing the port,
						// creates one
						sprintf( buffer, "select port_%s_id from object_%d", name, i );
						sqlite3_exec( db, buffer, callback, 0, &error );
						sprintf( buffer, "no such column: port_%s_id", name );
						if( strcmp( error, buffer ) == 0 )
						{
							sprintf( buffer, "alter table object_%d add port_%s_id default NULL references object_%d_port_%s( id )", i, name, i, name );
							sqlite3_exec( db, buffer, callback, 0, &error );
							if(error)
								puts(error);
						}
						foo(fields, i, name);
						printf("12\n");
//						for( int j = 0; j < fields.size(); j++)
//						{
//							printf("%s\n", fields[j]->getOwner()->getMembers()[fields[j]->getIndex()]->getName().c_str() );
//							printf("%s\n", fields[j]->getName().c_str() );
//							printf("%s\n", fields[j]->getType()->getName().c_str() );
//						}

						printf("21\n");
					}
					printf("a\n");
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
			static int callback(void *NotUsed, int argc, char **argv, char **azColName){
				int i;
				for(i=0; i<argc; i++){
					printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
				}
				printf("\n");
				return 0;
			}
			//recursive function that 
			//creates a table for the object, and for each port if its a basic type creates a column
			//if not creates a foreign key column and call the function for that port
			void foo( const co::RefVector<co::IField>& fields, int object, const char* portName )
			{	
				char buffer[100];
				char* error;
				printf("entrou\n");
				for(int i = 0; i < fields.size(); i++) {
					printf("i - %d\n", i);
					if( fields[i]->getType()->getName() == "string" ) {
						sprintf( buffer, "alter table object_%d_port_%s add %s", object, portName, fields[i]->getName().c_str() );
						sqlite3_exec( db, buffer, callback, 0, &error );
						if( error )
							puts( error );	
					}
					else
					{
						sprintf( buffer, "create table object_%d_port_%s_field_%s ( id integer not null primary key )",
							       	object, portName, fields[i]->getName().c_str() );
						sqlite3_exec( db, buffer, callback, 0, &error );
						if( error )
							puts( error );
						sprintf( buffer, "alter table object_%d_port_%s add %s \
							       	default NULL references object_%d_port_%s_field_%s( id )",
							       	object, portName, fields[i]->getName().c_str(), object, portName,
							       	fields[i]->getName().c_str() );
						sqlite3_exec( db, buffer, callback, 0, &error );
						if( error )
							puts( error );	
						co::RefVector<co::IField> fields2;
						printf("É AQUI\n");
						sprintf( buffer, "%s_field_%s", portName, fields[i]->getName().c_str() );
						printf("é aqui.\n");
						_model->getFields( static_cast<co::IRecordType*>( fields[i]->getType() ), fields2 );
						printf("provado.\n");
						fflush(stdout);
//						foo( fields2, object, buffer );
					}
					printf("final\n");
				}
			}
	};

	CORAL_EXPORT_COMPONENT( SpaceSaverSQLite3, SpaceSaverSQLite3 );

} // namespace ca
