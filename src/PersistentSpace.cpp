#include "SpaceSaverSQLite3_Base.h"
#include <ca/ISpaceChanges.h>
#include <ca/IUniverse.h>
#include <ca/ISpace.h>
#include <ca/IModel.h>
#include <ca/InvalidSpaceFileException.h>
#include <ca/FormatException.h>

#include <ca/ISpaceStore.h>

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

#include <ca/StoredType.h>
#include <ca/StoredFieldValue.h>
#include <ca/StoredField.h>

#include "StringSerializer.h"
#include "SpaceSaverSQLQueries.h"
#include "AnyArrayUtil.h"
#include "IDBConnection.h"
#include "IResultSet.h"
#include "DBException.h"
#include "PersistentSpace_Base.h"

using namespace std;


namespace ca {

	class PersistentSpace : public PersistentSpace_Base
	{
		public:
			PersistentSpace()
			{
				
			}

			virtual ~PersistentSpace()
			{
				if(_spaceStore.isValid())
				{
					_spaceStore->close();
				}
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
				
				if( !_spaceStore.isValid() )
				{
					throw co::IllegalStateException("space file was not set, could not setup");
				}

				_spaceStore->open();

				try
				{
					saveObject(_space->getRootObject());
				}
				catch(...)
				{
					_spaceStore->close();
				}
				
				_spaceStore->close();

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

				if( !_spaceStore.isValid() )
				{
					throw co::IllegalStateException("space file was not set, can't restore a space");
				}

				_spaceStore->open();

				try
				{
					restoreObject(1);

					co::IObject* object = (co::IObject*)getRef(1);
				
					co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
					ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

					universeObj->setService( "model", _model.get() );
					co::IObject* spaceObj = co::newInstance( "ca.Space" );
					ca::ISpace* space = spaceObj->getService<ca::ISpace>();

					spaceObj->setService( "universe", universe );
					space->setRootObject( object );
					_spaceStore->close();
					return space;
				}
				catch(ca::InvalidSpaceFileException e)
				{
					_spaceStore->close();
					throw e;
				}
				
			}

			void saveChanges()
			{
				// TODO: implement this method.
			}
		protected:
			ca::ISpaceStore* getSpaceStoreService()
			{
				return _spaceStore.get();
			}

			void setSpaceStoreService( ca::ISpaceStore* spaceStore )
			{
				assert( spaceStore );
				_spaceStore = spaceStore;
			}
		private:

			co::RefPtr<ca::ISpace> _space;
			co::RefPtr<ca::IModel> _model;
			co::int32 _modelVersion;
			StringSerializer _serializer;

			co::RefPtr<ca::ISpaceStore> _spaceStore;

			map<void*, int> refMap;

			map<int, co::IObject*> idMap;

			map<std::string, int> _insertedTypeCache;

			co::RefVector<ca::ISpaceChanges> spaceChanges; 

			//save functions

			void insertRefMap(void* obj, int id)
			{
				refMap.insert(pair<void*, int>(obj, id));
			}

			void insertTypeCache(std::string type, int id)
			{
				_insertedTypeCache.insert(pair<std::string, int>(type, id));
			}

			void savePort(int componentId, co::IPort* port)
			{
				int typeId = _spaceStore->getOrAddType( port->getType()->getFullName(), 1 );
				
				saveField(port->getName(), componentId, typeId);
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

				int entityId = _spaceStore->getOrAddType( typeFullName, getCalciumModelId() );
				objId = _spaceStore->addObject( entityId );

				insertRefMap(obj.get(), objId);

				co::RefVector<co::IField> fields;
				_model->getFields(type, fields);
				
				ca::StoredType storedType;

				_spaceStore->getStoredType( entityId, storedType );
				int i = 0;
				std::vector<ca::StoredFieldValue> values;
				std::string fieldValueStr;
				for(co::RefVector<co::IField>::iterator it = fields.begin(); it != fields.end(); it++) 
				{
					co::IField* field =  it->get();
					std::string fieldName = field->getName();
					
					int fieldId = storedType.fields[i].fieldId;
					int fieldTypeId = storedType.fields[i].typeId;
					
					co::IReflector* reflector = field->getOwner()->getReflector();
					co::Any fieldValue;

					reflector->getField(obj.get(), field, fieldValue);

					std::string q = field->getType()->getFullName();
					
					co::TypeKind tk = fieldValue.getKind();

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
							_serializer.toString( getRefId(service->getProvider()), fieldValueStr );
						}
						
					}
					else
					{
						fieldValueStr = getValueAsString(fieldValue);
					}
					ca::StoredFieldValue value;
					value.fieldId = fieldId;
					value.value = fieldValueStr;
					values.push_back( value );
					i++;

				}
				_spaceStore->addValues( objId, 1, values );
			}

			bool needsToSaveType( co::IType* type )
			{
				return ( getInsertedTypeId(type->getFullName()) == -1 );
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

				int typeId = _spaceStore->getOrAddType( type->getFullName(), getCalciumModelId() );
				insertTypeCache( type->getFullName(), typeId );
			}

			void saveField(const std::string& fieldName, int entityId, int fieldTypeId)
			{
				_spaceStore->addField( entityId, fieldName, fieldTypeId );
			}

			void saveInterface(co::IInterface* entityInterface)
			{

				int getIdFromSavedEntity = _spaceStore->getOrAddType( entityInterface->getFullName(), getCalciumModelId() );
				insertTypeCache( entityInterface->getFullName(), getIdFromSavedEntity );

				co::RefVector<co::IField> fields;
				_model->getFields(entityInterface, fields);
				
				for(co::RefVector<co::IField>::iterator it = fields.begin(); it != fields.end(); it++) 
				{
					co::IField* field =  it->get();
					
					std::string typeFullName = field->getType()->getFullName();

					if(field->getType()->getKind() == co::TK_ARRAY)
					{
						co::IType* arrayType = static_cast<co::IArray*>(field->getType())->getElementType();
						
						saveEntity(arrayType);
					}
					if(field->getType()->getKind() == co::TK_INTERFACE || field->getType()->getKind() == co::TK_NATIVECLASS)
					{
						saveEntity(field->getType());
					}
					int fieldTypeId = _spaceStore->getOrAddType( typeFullName, getCalciumModelId() );
					saveField(field->getName(), getIdFromSavedEntity, fieldTypeId);
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

				int entityId = _spaceStore->getOrAddType(component->getFullName(), getCalciumModelId());

				ca::StoredType storedType;
				
				int objId = _spaceStore->addObject( entityId );

				insertRefMap(object.get(), objId);

				co::RefVector<co::IPort> ports;
				_model->getPorts(component, ports);

				_spaceStore->getStoredType( entityId, storedType );

				int fieldId;
				std::vector<ca::StoredFieldValue> values;
				for( int i = 0; i < ports.size(); i++ )
				{
					co::IPort* port = (ports[i]).get();
					co::IService* service = object->getServiceAt(port);

					assert( storedType.fields[i].fieldName == port->getName() );

					fieldId = storedType.fields[i].fieldId;

					std::string refStr;
					if(port->getIsFacet())
					{
						saveService(service, port);
						stringstream insertValue;
						
						_serializer.toString( getRefId( service ), refStr);
					}
					else
					{
						_serializer.toString( getRefId( service->getProvider() ), refStr);
					}
					ca::StoredFieldValue value;
					value.fieldId = fieldId;
					value.value = refStr;
					values.push_back( value );
					
				}
				_spaceStore->addValues( objId, 1, values );

			}

			void saveComponent(co::IComponent* component)
			{
				int componentId = _spaceStore->getOrAddType( component->getFullName(), getCalciumModelId() );
				insertTypeCache( component->getFullName(), componentId );
				co::Range<co::IPort* const> ports = component->getPorts();

				for( int i = 0; i < ports.getSize(); i++ )
				{
					savePort( componentId, ports[i] );
				}
			}

			int getInsertedTypeId( std::string typeFullName )
			{
				map<std::string,int>::iterator it = _insertedTypeCache.find( typeFullName );

				if(it != _insertedTypeCache.end())
				{
					return it->second;
				}

				return -1;
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
				int typeId = _spaceStore->getObjectType( id );

				ca::StoredType type;

				_spaceStore->getStoredType( typeId, type );

				std::string entity = type.typeName;
				
				co::IObject* object = co::newInstance( entity );
				
				co::RefVector<co::IPort> ports;
				_model->getPorts( object->getComponent(), ports );

				insertIdMap( id, object );

				co::IPort* port;
				std::string portName;

				std::vector<ca::StoredFieldValue> fieldValues;
				_spaceStore->getValues( id, 1, fieldValues );
				
				map<int, std::string> mapFieldValue;
				map<std::string, int> mapFieldIdName;
				
				for( int i = 0; i < type.fields.size(); i++ )
				{
					ca::StoredField field = type.fields[i];
					mapFieldIdName.insert( pair<std::string, int>( field.fieldName, field.fieldId ) );
				}

				for( int i = 0; i < fieldValues.size(); i++ )
				{
					ca::StoredFieldValue fieldValue = fieldValues[i];
					mapFieldValue.insert( pair<int, std::string>( fieldValue.fieldId, fieldValue.value ) );
				}


				for( int i = 0; i < ports.size(); i++ )
				{
					port = ports.at(i).get();
					portName = port->getName();
					
					int fieldId = mapFieldIdName.find(portName)->second;
					int idService = atoi( mapFieldValue.find(fieldId)->second.c_str() );

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
		
				std::vector<ca::StoredFieldValue> fieldValues;

				_spaceStore->getValues( id, getCalciumModelId(), fieldValues );

				ca::StoredType type;
				int typeId = _spaceStore->getOrAddType( service->getInterface()->getFullName(), getCalciumModelId() );
				_spaceStore->getStoredType( typeId, type );

				map<int, std::string> mapFieldValue;
				map<std::string, int> mapFieldIdName;
				
				for( int i = 0; i < type.fields.size(); i++ )
				{
					ca::StoredField field = type.fields[i];
					mapFieldIdName.insert( pair<std::string, int>( field.fieldName, field.fieldId ) );
				}

				for( int i = 0; i < fieldValues.size(); i++ )
				{
					ca::StoredFieldValue fieldValue = fieldValues[i];
					mapFieldValue.insert( pair<int, std::string>( fieldValue.fieldId, fieldValue.value ) );
				}

				co::RefVector<co::IField> fields;
				_model->getFields( service->getInterface(), fields );

				co::IReflector* ref = service->getInterface()->getReflector();
				std::string fieldName;
				for( int i = 0; i < fields.size(); i++)
				{
					co::IField* field = fields[i].get();
					fieldName = field->getName();
					int fieldId = mapFieldIdName.find(fieldName)->second;
					fillFieldValue(service, field, id, mapFieldValue.find(fieldId)->second);
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
						_serializer.fromString( strValue, co::typeOf<co::int32>::get(), refId );
						int refIdInt = refId.get<co::int32>();
						co::IObject* ref = getRef( refIdInt );

						fieldValue.set(ref->getServiceAt( getFirstFacet(ref, field->getType()) ));

					}

				}
				else
				{
					try
					{
						_serializer.fromString(strValue, field->getType(), fieldValue);
					}
					catch( ca::FormatException e )
					{
						stringstream ss;
						ss << "Invalid field value for field " << field->getName();
						throw ca::FormatException( ss.str() );
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
					throw ca::InvalidSpaceFileException( ss.str() );
				}

			}

			void fillInterfaceArrayValue( std::string strValue, co::IArray* arrayType, co::Any& arrayValue )
			{
				assert( arrayType->getElementType()->getKind() == co::TK_INTERFACE );

				co::Any refs;
				_serializer.fromString(strValue, co::typeOf<std::vector<co::int32>>::get(), refs);

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

			int getCalciumModelId()
			{
				return 1;
			}

			std::string getValueAsString(co::Any& value)
			{
				
				std::string result;
				_serializer.toString(value, result);

				if( value.getKind() == co::TK_STRING && result[0] == '\'' )
				{
					result.insert( 0, "\'" );
					result.push_back('\'');
				}

				return result;
			}

	};

	CORAL_EXPORT_COMPONENT( PersistentSpace, PersistentSpace );
	

} // namespace ca
