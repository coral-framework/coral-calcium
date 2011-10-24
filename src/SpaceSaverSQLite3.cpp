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
#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>
#include <sstream>

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
				
				co::RefPtr<co::IObject> serializerObj = co::newInstance("ca.StringSerializer");
				_serializer = serializerObj->getService<ca::IStringSerializer>();

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
				co::Any result;
				
				co::RefPtr<co::IObject> serializerObj = co::newInstance("ca.StringSerializer");
				_serializer = serializerObj->getService<ca::IStringSerializer>();

				if( _db != 0 )
				{
					_db->close();
				}

				co::RefPtr<co::IObject> db = co::newInstance("ca.SQLiteDBConnection");

				_db = db->getService<ca::IDBConnection>();
				_db->getProvider()->getService<ca::INamed>()->setName(_fileName);

				_db->open();
				restoreObject(1);

				co::IObject* object = (co::IObject*)getRef(1);
				
				// TODO: implement this method.
				co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
				ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

				universeObj->setService("model", _model.get());
				co::IObject* spaceObj = co::newInstance("ca.Space");
				ca::ISpace* space = spaceObj->getService<ca::ISpace>();

				spaceObj->setService( "universe", universe );
				space->setRootObject( object );
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
			ca::IStringSerializer* _serializer;

			map<void*, int> refMap;

			map<int, co::IObject*> idMap;

			std::string _fileName;

			co::RefVector<ca::ISpaceChanges> spaceChanges; 

			char *error;
			char buffer[500];

			void restoreObject( int id )
			{
				stringstream sql;

				sql << "SELECT E.ENTITY_NAME, E.ENTITY_ID FROM OBJECT OBJ, ENTITY E WHERE OBJ.ENTITY_ID = E.ENTITY_ID AND OBJ.OBJECT_ID = " << id << " AND E.CAMODEL_ID = " << getCalciumModelId();

				ca::IResultSet* rs = _db->executeQuery( sql.str() );
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
					
					sql.flush();
					sql.clear();
					sql.str("");
					sql << "SELECT FV.VALUE FROM FIELD F LEFT OUTER JOIN FIELD_VALUES FV ON F.FIELD_ID = FV.FIELD_ID WHERE F.FIELD_NAME = '"
						<< portName <<"' AND FV.OBJECT_ID = " << id << " AND F.ENTITY_ID = " << entity_id;

					rs = _db->executeQuery( sql.str() );

					if( !rs->next() )
					{
						std::string sqlMsg = sql.str();
						printf( sqlMsg.c_str() );
					}
					
					int idService = atoi( rs->getValue(0).c_str() );
					rs->finalize();

					if( port->getIsFacet() )
					{

						co::IService* service = object->getService( port );

						fillServiceValues( idService, service );
					}
					else 
					{
						co::IObject* refObj = getRef( idService );
						try
						{
							object->setService( port, refObj->getService( getFirstFacet(refObj) ) );
						}
						catch( co::IllegalCastException e )
						{
							printf( e.getMessage().c_str() );
						}
					}

				}
			}

			co::IPort* getFirstFacet( co::IObject* refObj )
			{
				co::RefVector< co::IPort > ports;
				_model->getPorts( refObj->getComponent(), ports );

				for( int i = 0; i < ports.size(); i++ )
				{
					if( ports[i].get()->getIsFacet() )
					{
						return ports[i].get();
					}
				}
				return 0;
			}

			void fillServiceValues( int id, co::IService* service )
			{
				stringstream sql;
				sql << "SELECT F.FIELD_NAME, FV.VALUE FROM " <<
					"FIELD_VALUES FV LEFT OUTER JOIN OBJECT OBJ ON FV.OBJECT_ID = OBJ.OBJECT_ID "<<
					"LEFT OUTER JOIN ENTITY E ON OBJ.ENTITY_ID = E.ENTITY_ID " <<
					"LEFT OUTER JOIN FIELD F ON F.ENTITY_ID = E.ENTITY_ID AND F.FIELD_ID = FV.FIELD_ID " <<
					"LEFT OUTER JOIN CALCIUM_MODEL CM ON E.CAMODEL_ID = CM.CAMODEL_ID "<<
					"WHERE CM.CAMODEL_ID = 1 AND FV.FIELD_VALUE_VERSION = 1 " <<
					"AND OBJ.OBJECT_ID = " << id;

				ca::IResultSet* rs = _db->executeQuery( sql.str() );
				
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
					extractFieldValue(service, field, id, mapFieldValue.find(fieldName)->second);
				}


			}

			void extractFieldValue( co::IService* service, co::IField* field, int id, std::string strValue )
			{
				co::IType* type = field->getType();
				
				co::IReflector* reflector = service->getInterface()->getReflector();

				co::Any serviceAny;
				serviceAny.setService( service, service->getInterface() );

				if( type->getKind() == co::TK_ARRAY )
				{
					co::IArray* arrayType = static_cast<co::IArray*>(type);

					if( arrayType->getElementType()->getKind() == co::TK_INTERFACE )
					{
						co::Any refs;
						_serializer->fromString(strValue, co::typeOfArrayBase<co::int32>::get(), refs);

						std::vector<co::int32> vec = refs.get<const std::vector<co::int32>&>();
						std::vector<co::IService*> services;

						co::Any* fieldValue = new co::Any();;

						fieldValue->createArray( arrayType->getElementType(), vec.size() );

						co::IObject* ref;
						for( int i = 0; i < vec.size(); i++ )
						{
							ref = getRef( vec[i] );
							
							if( ref == 0 )
							{
								restoreObject( vec[i] );
								ref = getRef(vec[i]);
								co::IPort* port = getFirstFacet(ref);	
								std::string componentName = ref->getComponent()->getName();
								co::IService* service = ref->getService(port);
								try
								{
									setArrayComplexTypeElement( *fieldValue, i, co::Any(service) );
								}
								catch ( co::NotSupportedException e )
								{
									printf(e.getMessage().c_str());
								}
								
							}
						}

						try
						{
							reflector->setField( serviceAny, field, *fieldValue );
						}
						catch( co::IllegalCastException e )
						{
							
							printf(e.getMessage().c_str());
						}

						return;
					}
					
				}
				if( type->getKind() == co::TK_INTERFACE )
				{
					if( strValue != "nil" )
					{
						co::Any refId;
						_serializer->fromString(strValue, co::typeOf<co::int32>::get(), refId);
						int refIdInt = refId.get<co::int32>();
						co::IObject* ref = getRef( refIdInt );
						if( ref == 0 )
						{
							restoreObject( refIdInt );
						}
						ref = getRef( refIdInt );
						try 
						{
							reflector->setField( service, field, ref->getService( getFirstFacet(ref) ) );
						}
						catch (co::IllegalCastException e)
						{
							printf( e.getMessage().c_str() );
						}
					}

					return;
				}

				co::Any anyValue;

				_serializer->fromString(strValue, field->getType(), anyValue);
				try
				{
					reflector->setField( serviceAny, field, anyValue );
				}
				catch (co::IllegalCastException e)
				{
					printf( e.getMessage().c_str() );
				}

				printf( "%s %s %s\n", service->getInterface()->getFullName().c_str(), field->getName().c_str(), strValue.c_str() );

			}

			void insertIdMap( int id, co::IObject* obj )
			{
				idMap.insert( pair<int, co::IObject*>( id, obj ) );
			}

			void insertRefMap(void* obj, int id)
			{
				refMap.insert(pair<void*, int>(obj, id));
			}

			co::IObject* getRef(int id)
			{
				map<int, co::IObject*>::iterator it = idMap.find(id);

				if(it != idMap.end())
				{
					return it->second;
				}

				return 0;
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

			int getEntityId(std::string entityName, int calciumModelId)
			{
				sprintf(buffer, "SELECT ENTITY_ID FROM ENTITY WHERE ENTITY_NAME = '%s' AND CAMODEL_ID = %i", entityName.c_str(), calciumModelId);

				co::RefPtr<ca::IResultSet> rs = _db->executeQuery(buffer);
				if(rs->next())
				{
					return atoi(rs->getValue(0).c_str());
				}
				rs->finalize();
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
				rs->finalize();
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
				rs->finalize();
				return -1;
			}


			void createModelMetadata()
			{
				saveObject(_space->getRootObject());
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

				sprintf(buffer, "INSERT INTO OBJECT (ENTITY_ID) VALUES (%i)", entityId);
				_db->execute(buffer);
			
				co::RefPtr<ca::IResultSet> rs = _db->executeQuery("SELECT MAX(OBJECT_ID) FROM OBJECT");
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
					co::IService* service = object->getService(port);

					fieldId = getFieldId( port->getName(), entityId );

					if(port->getIsFacet())
					{
						saveService(service, port);
						stringstream insertValue;
						insertValue << "INSERT INTO FIELD_VALUES (FIELD_ID, OBJECT_ID, FIELD_VALUE_VERSION, VALUE) VALUES (" << fieldId
							<< ", " << objId << ", " << 1 << ", '" << getRefId( service ) << "')";
						
						_db->execute(insertValue.str().c_str());
					}
					else
					{
						stringstream insertValue;
						insertValue << "INSERT INTO FIELD_VALUES (FIELD_ID, OBJECT_ID, FIELD_VALUE_VERSION, VALUE) VALUES (" << fieldId
							<< ", " << objId << ", " << 1 << ", '" << getRefId( service->getProvider() ) << "')";
						_db->execute(insertValue.str().c_str());
					}
					
				}

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

				sprintf(buffer, "INSERT INTO OBJECT (ENTITY_ID) VALUES (%i)", getEntityId(typeFullName, getCalciumModelId()));
				_db->execute(buffer);
				co::RefPtr<ca::IResultSet> rs = _db->executeQuery("SELECT MAX(OBJECT_ID) FROM OBJECT");
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
					else if (tk == co::TK_INTERFACE)
					{
						co::IService* const service = fieldValue.get<co::IService* const>();
						if(service == 0)
						{
							fieldValueStr = "nil";	
						}
						else
						{
							saveObject(service->getProvider());
							char valueFromInt[5];
							sprintf(valueFromInt, "%i", getRefId(service->getProvider())); 
							fieldValueStr.assign(valueFromInt);
						}
						
					}
					else
					{
						fieldValueStr = getValueAsString(fieldValue);
					}

					sprintf(buffer, "INSERT INTO FIELD_VALUES (FIELD_ID, OBJECT_ID, FIELD_VALUE_VERSION, VALUE) VALUES (%i, %i, %i, '%s')", fieldId, objId, 1, fieldValueStr.c_str());
					_db->execute(buffer);

				}
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

			void saveComponent(co::IComponent* component)
			{
				std::string compName = component->getFullName();
				sprintf(buffer, "INSERT INTO ENTITY (ENTITY_NAME, CAMODEL_ID) VALUES ('%s', %i)", compName.c_str(), getCalciumModelId());
				_db->execute(buffer);

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
				std::string typeName = type->getFullName();
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

			co::uint32 getArraySize( const co::Any& array )
			{
				assert( array.getKind() == co::TK_ARRAY );

				const co::Any::State& s = array.getState();
				if( s.arrayKind == co::Any::State::AK_Range )
					return s.arraySize;

				co::Any::PseudoVector& pv = *reinterpret_cast<co::Any::PseudoVector*>( s.data.ptr );
				return static_cast<co::uint32>( pv.size() ) / array.getType()->getReflector()->getSize();
			}

			co::uint8* getArrayPtr( const co::Any& array )
			{
				assert( array.getKind() == co::TK_ARRAY );

				const co::Any::State& s = array.getState();
				if( s.arrayKind == co::Any::State::AK_Range )
					return reinterpret_cast<co::uint8*>( s.data.ptr );

				co::Any::PseudoVector& pv = *reinterpret_cast<co::Any::PseudoVector*>( s.data.ptr );
				return &pv[0];
			}

			void getArrayElement( const co::Any& array, co::uint32 index, co::Any& element )
			{
				co::IType* elementType = array.getType();
				co::uint32 elementSize = elementType->getReflector()->getSize();

				co::TypeKind ek = elementType->getKind();
				bool isPrimitive = ( ( ek >= co::TK_BOOLEAN && ek <= co::TK_DOUBLE ) || ek == co::TK_ENUM );
				co::uint32 flags = isPrimitive ? co::Any::VarIsValue : co::Any::VarIsConst|co::Any::VarIsReference;

				void* ptr = getArrayPtr( array ) + elementSize * index;

				element.setVariable( elementType, flags, ptr );
			}

			template< typename T>
			void setArrayElement( const co::Any& array, co::uint32 index, T element )
			{
				co::IType* elementType = array.getType();
				co::uint32 elementSize = elementType->getReflector()->getSize();

				co::uint8* p = getArrayPtr(array);

				co::TypeKind ek = elementType->getKind();
				bool isPrimitive = ( ( ek >= co::TK_BOOLEAN && ek <= co::TK_DOUBLE ) || ek == co::TK_ENUM );
				co::uint32 flags = isPrimitive ? co::Any::VarIsValue : co::Any::VarIsConst|co::Any::VarIsReference;

				void* ptr = getArrayPtr( array ) + elementSize * index;
			
				T* elementTypePointer = reinterpret_cast<T*>(ptr);
				*elementTypePointer = element;
			}

			void setArrayComplexTypeElement( const co::Any& array, co::uint32 index, co::Any& element )
			{
				co::IType* elementType = array.getType();
				co::IReflector* reflector = elementType->getReflector();
				co::uint32 elementSize = reflector->getSize();

				co::uint8* p = getArrayPtr(array);

				co::TypeKind ek = elementType->getKind();
				bool isPrimitive = ( ( ek >= co::TK_BOOLEAN && ek <= co::TK_DOUBLE ) || ek == co::TK_ENUM );
				co::uint32 flags = isPrimitive ? co::Any::VarIsValue : co::Any::VarIsConst|co::Any::VarIsReference;

				void* ptr = getArrayPtr( array ) + elementSize * index;
				if( !element.isPointer())
				{
					reflector->copyValue( element.getState().data.ptr, ptr );
				}
				else
				{
					ptr = element.getState().data.service;
				}

			}

	};

	CORAL_EXPORT_COMPONENT( SpaceSaverSQLite3, SpaceSaverSQLite3 );
	

} // namespace ca
