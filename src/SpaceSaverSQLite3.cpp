#include "SpaceSaverSQLite3_Base.h"
#include <ca/ISpaceChanges.h>
#include <ca/IUniverse.h>
#include <ca/ISpace.h>
#include <ca/IModel.h>
#include <ca/IDBConnection.h>
#include <ca/IResultSet.h>
#include <ca/DBException.h>
#include <ca/IStringSerializer.h>
#include <ca/MalformedSerializedStringException.h>
#include <ca/InvalidSpaceFileException.h>

#include "sqlite3.h"
#include <co/RefVector.h>
#include <co/IReflector.h>
#include <co/RefPtr.h>
#include <co/IObject.h>
#include <co/IField.h>
#include <co/Coral.h>
#include <co/IArray.h>
#include <co/Any.h>
#include <co/IComponent.h>
#include <co/IPort.h>
#include <co/IllegalArgumentException.h>
#include <co/NotSupportedException.h>
#include <co/IllegalCastException.h>
#include <co/IllegalArgumentException.h>
#include <co/IllegalStateException.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>
#include <sstream>

#include "SpaceSaverSQLQueries.h"
#include "AnyArrayUtil.h"

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
				closeDBConnection();
			}

			// ------ ca.ISpaceObserver Methods ------ //

			void onSpaceChanged( ca::ISpaceChanges* changes )
			{
				spaceChanges.push_back( changes );
			}

			void setup()
			{
				if( !_space )
				{
					throw co::IllegalStateException("space unset, could not setup");
				}
				
				co::RefPtr<co::IObject> serializerObj = co::newInstance("ca.StringSerializer");
				_serializer = serializerObj->getService<ca::IStringSerializer>();

				try
				{
					getDBConnection()->createDatabase();
				}
				catch( DBException e )
				{
					throw co::IllegalStateException( "Could not create database file" );
				}
				

				createTables();

				saveObject(_space->getRootObject());

				closeDBConnection();

			}

			void setSpace( ca::ISpace* space )
			{
				if( space == NULL )
				{
					throw co::IllegalArgumentException( "space can't be NULL" );
				}
				_space = space;
				ca::IUniverse* universe = static_cast<ca::IUniverse*>(_space->getProvider()->getService("universe"));
				
				assert(universe);
				_model = static_cast<ca::IModel*>( universe->getProvider()->getService("model"));
				assert(_model);

				_modelVersion = 1;//_model->getVersion();
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
				if( !_space )
				{
					throw co::IllegalStateException("space was not set, model definition missing, can't restore a space");
				}

				co::RefPtr<co::IObject> serializerObj = co::newInstance("ca.StringSerializer");
				_serializer = serializerObj->getService<ca::IStringSerializer>();

				if( _db.get() )
				{
					closeDBConnection();
				}

				getDBConnection()->open();
				restoreObject(1);

				co::IObject* object = (co::IObject*)getRef(1);
				
				co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
				ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

				universeObj->setService("model", _model.get());
				co::IObject* spaceObj = co::newInstance("ca.Space");
				ca::ISpace* space = spaceObj->getService<ca::ISpace>();

				spaceObj->setService( "universe", universe );
				space->setRootObject( object );

				closeDBConnection();

				return space;
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
			co::RefPtr<ca::IStringSerializer> _serializer;

			map<void*, int> refMap;

			map<int, co::IObject*> idMap;

			std::string _fileName;

			co::RefVector<ca::ISpaceChanges> spaceChanges; 

			// aux functions
			co::RefPtr<ca::IDBConnection> getDBConnection()
			{
				if(!_db)
				{
					co::RefPtr<co::IObject> db = co::newInstance("ca.SQLiteDBConnection");

					_db = db->getService<ca::IDBConnection>();
					_db->getProvider()->getService<ca::INamed>()->setName(_fileName);
				}
				return _db;
			}

			void closeDBConnection()
			{
				getDBConnection()->close();
				_db = NULL;
			}

			//save functions

			void insertRefMap(void* obj, int id)
			{
				refMap.insert(pair<void*, int>(obj, id));
			}

			void savePort(co::IComponent* component, co::IPort* port)
			{
				int componentId = getEntityId(component->getFullName(), getCalciumModelId()); // estrutura de dados pra armazenar entities já inseridas

				saveField(port->getName(), componentId, port->getType()->getFullName());
				
				saveEntity(port->getType());

			}

			void saveService(co::RefPtr<co::IService> obj, co::IPort* port)
			{
				int objId = getRefId(obj.get());
				if(objId != -1)
				{
					return;
				}

				co::IInterface* type = port->getType();
				saveEntity(type);

				std::string typeFullName = type->getFullName();

				int entityId = getEntityId(typeFullName, getCalciumModelId());

				executeOrThrow( SpaceSaverSQLQueries::insertObject( entityId ) );

				co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectLastInsertedObject() );
				rs->next();
				objId = atoi(rs->getValue(0).c_str());
				insertRefMap(obj.get(), objId);
				rs->finalize();

				co::RefVector<co::IField> fields;
				_model->getFields(type, fields);
				
				for(co::RefVector<co::IField>::iterator it = fields.begin(); it != fields.end(); it++) 
				{
					co::IField* field =  it->get();
					std::string fieldName = field->getName();
					
					int fieldId = getFieldId(fieldName, entityId);
					
					co::IReflector* reflector = field->getOwner()->getReflector();
					co::Any fieldValue;

					try{
						reflector->getField(obj.get(), field, fieldValue);
					}
					catch(co::IllegalArgumentException e)
					{
						std::string error = e.getMessage();
						error.c_str();
					}

					std::string q = field->getType()->getFullName();
					int i = field->getKind();
					co::TypeKind tk = fieldValue.getKind();

					std::string fieldValueStr;
					if(tk == co::TK_ARRAY)
					{
						co::IType* arrayType = static_cast<co::IArray*>(field->getType())->getElementType();
												
						if(arrayType->getKind() == co::TK_INTERFACE)
						{
							co::Range<co::IService* const> services = fieldValue.get<co::Range<co::IService* const>>();
							
							vector<co::int32> refs;
							while(!services.isEmpty())
							{
								co::IService* const serv = services.getFirst();
								std::string strInt = serv->getInterface()->getFullName();
								saveObject(serv->getProvider());
								
								int refId = getRefId(serv->getProvider());
								
								refs.push_back(refId);
								services.popFirst();
							
							}
							co::Any refsValue;
							refsValue.set<const std::vector<co::int32>&>(refs);
							fieldValueStr = getValueAsString(refsValue);
						}
						else
						{
							fieldValueStr = getValueAsString( fieldValue );
						}
						
					}
					else if ( tk == co::TK_INTERFACE )
					{
						co::IService* const service = fieldValue.get<co::IService* const>();
						if(service == NULL)
						{
							fieldValueStr = "nil";
						}
						else
						{
							saveObject(service->getProvider());
							_serializer->toString( getRefId(service->getProvider()), fieldValueStr );
						}
						
					}
					else
					{
						fieldValueStr = getValueAsString(fieldValue);
					}

					executeOrThrow( SpaceSaverSQLQueries::insertFieldValue( fieldId, objId, 1, fieldValueStr ) );

				}
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
				executeOrThrow( SpaceSaverSQLQueries::insertField( fieldName, entityId, fieldType ) );
			}

			void saveInterface(co::IInterface* entityInterface)
			{
				executeOrThrow( SpaceSaverSQLQueries::insertEntity( entityInterface->getFullName(), getCalciumModelId() ) );

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

			void saveObject(co::RefPtr<co::IObject> object)
			{
				if(getRefId(object.get()) != -1)
				{
					return;
				}

				co::IComponent* component = object->getComponent();
				saveEntity(component);

				int entityId = getEntityId(component->getFullName(), getCalciumModelId());

				executeOrThrow( SpaceSaverSQLQueries::insertObject( entityId )  );
			
				co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectLastInsertedObject() );
				rs->next();

				int objId = atoi(rs->getValue(0).c_str());
				insertRefMap(object.get(), objId);
				rs->finalize();
				
				co::RefVector<co::IPort> ports;
				_model->getPorts(component, ports);

				int fieldId; 

				for(co::RefVector<co::IPort>::iterator it = ports.begin(); it != ports.end(); it++) 
				{
					co::IPort* port = (*it).get();
					co::IService* service = object->getServiceAt(port);

					fieldId = getFieldId( port->getName(), entityId );

					if(port->getIsFacet())
					{
						saveService(service, port);
						stringstream insertValue;
												
						executeOrThrow( SpaceSaverSQLQueries::insertRefFieldValue( fieldId, objId, 1, getRefId( service ) ) );
					}
					else
					{
						executeOrThrow( SpaceSaverSQLQueries::insertRefFieldValue( fieldId, objId, 1, getRefId(service->getProvider() ) ) );
					}
					
				}

			}

			void saveComponent(co::IComponent* component)
			{
				std::string compName = component->getFullName();
				executeOrThrow( SpaceSaverSQLQueries::insertEntity( compName, getCalciumModelId() ) );

				co::Range<co::IPort* const> ports = component->getPorts();

				for( int i = 0; i < ports.getSize(); i++ )
				{
					savePort( component, ports[i] );
				}
			}
			
			bool entityAlreadyInserted(co::IType* entityInterface)
			{
				return ( getEntityId(entityInterface->getFullName(), getCalciumModelId()) != -1 );
			}


			bool needsToSaveType(co::IType* type)
			{
				if(type->getKind() != co::TK_COMPONENT &&
				   type->getKind() != co::TK_INTERFACE)
				{
					return false; //primitive type
				}

				return !entityAlreadyInserted(type);
			}

			int getRefId(void* obj)
			{
				map<void*,int>::iterator it = refMap.find(obj);

				if(it != refMap.end())
				{
					return it->second;
				}

				return -1;
			}

			//restore functions

			co::IObject* getRef( int id )
			{
				map<int, co::IObject*>::iterator it = idMap.find(id);

				if(it != idMap.end())
				{
					return it->second;
				}
				else 
				{
					restoreObject( id );
					return getRef( id );

				}
			}

			void insertIdMap( int id, co::IObject* obj )
			{
				idMap.insert( pair<int, co::IObject*>( id, obj ) );
			}

			void restoreObject( int id )
			{
				co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectEntityFromObject( id, getCalciumModelId() ) );
				
				bool end = rs->next();
				std::string entity = rs->getValue(0);
				int entity_id = atoi(rs->getValue(1).c_str());
				rs->finalize();

				co::IObject* object = co::newInstance(entity);
				co::RefVector<co::IPort> ports;
				_model->getPorts( object->getComponent(), ports );

				insertIdMap( id, object );

				co::IPort* port;
				std::string portName;

				for( int i = 0; i < ports.size(); i++ )
				{
					port = ports.at(i).get();
					portName = port->getName();
					
					rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectFieldValue( portName, id, entity_id ) );

					if( !rs->next() )
					{
						stringstream ss;
						ss << "Could not restore object, expected value for port " << portName << "on entity " << entity << " not found ";
						closeDBConnection();
						throw ca::InvalidSpaceFileException( ss.str() );
					}
					
					int idService = atoi( rs->getValue(0).c_str() );
					rs->finalize();

					if( port->getIsFacet() )
					{

						co::IService* service = object->getServiceAt( port );
						fillServiceValues( idService, service );
					}
					else 
					{
						co::IObject* refObj = getRef( idService );
						try
						{
							object->setServiceAt( port, refObj->getServiceAt( getFirstFacet(refObj, port->getType()) ) );
						}
						catch( co::IllegalCastException e )
						{
							stringstream ss;
							ss << "Could not restore object, invalid value type for port " << portName << "on entity " << entity;
							closeDBConnection();
							throw ca::InvalidSpaceFileException( ss.str() );
						}
					}

				}
			}

			co::IPort* getFirstFacet( co::IObject* refObj, co::IType* type )
			{
				co::RefVector< co::IPort > ports;
				_model->getPorts( refObj->getComponent(), ports );

				for( int i = 0; i < ports.size(); i++ )
				{
					if( ports[i].get()->getIsFacet() && (ports[i]->getType()->getFullName() == type->getFullName()) )
					{
						return ports[i].get();
					}
				}
				return NULL;
			}

			void fillServiceValues( int id, co::IService* service )
			{
				SpaceSaverSQLQueries::selectFieldValues( id, 1, 1 );
				stringstream sql;
				
				co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectFieldValues( id, 1, 1 ) );
				
				map<std::string, std::string> mapFieldValue;

				while( rs->next() )
				{
					std::string fieldName = rs->getValue(0);
					std::string fieldValue = rs->getValue(1);
					mapFieldValue.insert( pair<std::string, std::string>( fieldName, fieldValue ) );
				}
				rs->finalize();
				co::RefVector<co::IField> fields;
				_model->getFields( service->getInterface(), fields );

				co::IReflector* ref = service->getInterface()->getReflector();
				std::string fieldName;
				for( int i = 0; i < fields.size(); i++)
				{
					co::IField* field = fields[i].get();
					fieldName = field->getName();
					fillFieldValue(service, field, id, mapFieldValue.find(fieldName)->second);
				}
			}

			void fillFieldValue( co::IService* service, co::IField* field, int id, std::string strValue )
			{
				co::IType* type = field->getType();
				co::IReflector* reflector = service->getInterface()->getReflector();

				co::Any serviceAny;
				serviceAny.setService( service, service->getInterface() );

				co::Any fieldValue;

				if( type->getKind() == co::TK_ARRAY )
				{
					co::IArray* arrayType = static_cast<co::IArray*>(type);

					if( arrayType->getElementType()->getKind() == co::TK_INTERFACE )
					{
						fillInterfaceArrayValue(strValue, arrayType, fieldValue);
					}
				
				}
				if( type->getKind() == co::TK_INTERFACE )
				{
					if( strValue != "nil" )
					{
						co::Any refId;
						_serializer->fromString( strValue, co::typeOf<co::int32>::get(), refId );
						int refIdInt = refId.get<co::int32>();
						co::IObject* ref = getRef( refIdInt );

						fieldValue.set(ref->getServiceAt( getFirstFacet(ref, field->getType()) ));

					}

				}
				else
				{
					try
					{
						_serializer->fromString(strValue, field->getType(), fieldValue);
					}
					catch( ca::MalformedSerializedStringException e )
					{
						stringstream ss;
						ss << "Invalid field value for field " << field->getName();
						closeDBConnection();
						throw ca::InvalidSpaceFileException( ss.str() );
					}
					
				}
				
				try
				{
					if( fieldValue.getKind() != co::TK_NONE )
					{
						reflector->setField( serviceAny, field, fieldValue );
					}

				}
				catch (co::IllegalCastException e)
				{
					stringstream ss;
					ss << "Invalid field value type for field " << field->getName() << "; type expected: " << field->getType()->getFullName();
					closeDBConnection();
					throw ca::InvalidSpaceFileException( ss.str() );
				}

			}

			void fillInterfaceArrayValue( std::string strValue, co::IArray* arrayType, co::Any& arrayValue )
			{
				assert( arrayType->getElementType()->getKind() == co::TK_INTERFACE );

				co::Any refs;
				_serializer->fromString(strValue, co::typeOf<std::vector<co::int32>>::get(), refs);

				std::vector<co::int32> vec = refs.get<const std::vector<co::int32>&>();
				co::RefVector<co::IService> services;

				arrayValue.createArray( arrayType->getElementType(), vec.size() );

				co::IObject* ref;
				for( int i = 0; i < vec.size(); i++ )
				{
					ref = getRef( vec[i] );
							
					co::IPort* port = getFirstFacet(ref, arrayType->getElementType());	
					co::IService* service = ref->getServiceAt(port);
								
					co::Any servicePtr;
					servicePtr.setService(service);
								
					try
					{
						AnyArrayUtil arrayUtil;
						arrayUtil.setArrayComplexTypeElement( arrayValue, i, servicePtr );
					}
					catch ( co::NotSupportedException e )
					{
						throw ca::InvalidSpaceFileException("Could not restore array");
					}
				}

			}

			int getEntityId(std::string entityName, int calciumModelId)
			{
				co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectEntityIdByName( entityName, calciumModelId ) );
				if(rs->next())
				{
					const char* entityIdStr = rs->getValue(0).c_str();
					rs->finalize();
					return atoi( entityIdStr );
				}
					
				return -1;
			}

			co::RefPtr<ca::IResultSet> executeQueryOrThrow( std::string sql )
			{
				try
				{
					co::RefPtr<ca::IResultSet> rs = _db->executeQuery( sql );
					return rs;
				}
				catch ( ca::DBException e )
				{
					closeDBConnection();
					throw ca::InvalidSpaceFileException("Unexpected database query exception");
				}		
				
			}

			void executeOrThrow( std::string sql )
			{
				try
				{
					_db->execute( sql );
					
				}
				catch ( ca::DBException e )
				{
					closeDBConnection();
					throw ca::InvalidSpaceFileException("Unexpected database query exception");
				}	
			}

			int getCalciumModelId()
			{
				std::string modelName = _model->getName();
				co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectCalciumModel( modelName, _modelVersion ) );
				if(rs->next())
				{
					return atoi(rs->getValue(0).c_str());
				}
				rs->finalize();
				return -1;
			}

			int getFieldId(std::string fieldName, int entityId)
			{
				co::RefPtr<ca::IResultSet> rs = executeQueryOrThrow( SpaceSaverSQLQueries::selectFieldIdByName( fieldName, entityId ) );
				if(rs->next())
				{
					return atoi(rs->getValue(0).c_str());
				}
				rs->finalize();
				return -1;
			}


			std::string getValueAsString(co::Any& value)
			{
				
				std::string result;
				_serializer->toString(value, result);

				if( value.getKind() == co::TK_STRING && result[0] == '\'' )
				{
					result.insert( 0, "\'" );
					result.push_back('\'');
				}

				return result;
			}

			void createTables()
			{
				
				try{
					_db->execute("BEGIN TRANSACTION");

					_db->execute("PRAGMA foreign_keys = 1");

					_db->execute( SpaceSaverSQLQueries::createTableCalciumModel() );

					_db->execute( SpaceSaverSQLQueries::createTableEntity() );
				
					_db->execute( SpaceSaverSQLQueries::createTableField() );

					_db->execute( SpaceSaverSQLQueries::createTableObject() );
					
					_db->execute( SpaceSaverSQLQueries::createTableFieldValues() );
				
					std::string modelName = _model->getName();
					_db->execute( SpaceSaverSQLQueries::insertCalciumModel( modelName, _modelVersion ));

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
