/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SpacePersister_Base.h"

#include <ca/ISpaceChanges.h>
#include <ca/IUniverse.h>
#include <ca/ISpace.h>
#include <ca/IOException.h>
#include <ca/FormatException.h>
#include <ca/ISpaceStore.h>

#include <co/RefVector.h>
#include <co/RefPtr.h>
#include <co/IObject.h>
#include <co/IField.h>
#include <co/IArray.h>
#include <co/Any.h>
#include <co/NotSupportedException.h>
#include <co/IllegalArgumentException.h>
#include <co/IllegalCastException.h>
#include <co/IllegalStateException.h>
#include <co/Log.h>

#include <set>
#include <map>

#include <ca/StoredType.h>
#include <ca/StoredFieldValue.h>
#include <ca/StoredField.h>

#include <ca/IObjectChanges.h>
#include <ca/ChangedConnection.h>
#include <ca/ChangedRefVecField.h>
#include <ca/ChangedRefField.h>
#include <ca/ChangedValueField.h>
#include <ca/IServiceChanges.h>

#include "StringSerializer.h"
#include "AnyArrayUtil.h"

namespace ca {

class SpacePersister : public SpacePersister_Base
{
public:
	SpacePersister()
	{
		_trackedRevision = 0;
		// empty
	}

	virtual ~SpacePersister()
	{
		if( _spaceStore.isValid() )
		{
			_spaceStore->close();
		}
		if( _space != NULL )
		{
			_space->removeSpaceObserver( this );
		}
	}

	// ------ ca.ISpaceObserver Methods ------ //

	void onSpaceChanged( ca::ISpaceChanges* changes )
	{
		_spaceChanges.push_back( changes );
	}

	// ------ ca.ISpacePersister Methods ------ //

	void initialize( co::IObject* rootObject )
	{
		assert( !_space );
		if( !_spaceStore.isValid() )
		{
			throw co::IllegalStateException("space file was not set, could not setup");
		}

		_spaceStore->open();

		try
		{
			_spaceStore->beginChanges();
			saveObject( rootObject );
			_spaceStore->commitChanges();
		}
		catch( ... )
		{
			_spaceStore->discardChanges();
			_spaceStore->close();
			throw;
		}
		
		co::IObject* spaceObj = co::newInstance( "ca.Space" );
		_space = spaceObj->getService<ca::ISpace>();

		spaceObj->setService( "universe", _universe.get() );
		
		_space->setRootObject( rootObject );
		_space->notifyChanges();

		_trackedRevision = 1;

		assert( _spaceStore->getLatestRevision() == 1 );

		_space->addSpaceObserver( this );

		_spaceStore->close();

	}

	ca::ISpace* getSpace()
	{
		return _space.get();
	}

	void restore()
	{
		assert( !_space );
		if( !_spaceStore.isValid() )
		{
			throw co::IllegalStateException("space store was not set, can't restore a space");
		}
		_spaceStore->open();
		co::uint32 latestRevision = _spaceStore->getLatestRevision();
		_spaceStore->close();

		restoreRevision( latestRevision );
		
	}

	void restoreRevision( co::uint32 revision )
	{
		assert( !_space );
		if( !_spaceStore.isValid() )
		{
			throw co::IllegalStateException("space store was not set, can't restore a space");
		}

		_spaceStore->open();

		if( _spaceStore->getLatestRevision() == 0 )
		{
			_spaceStore->close();
			throw co::IllegalArgumentException("empty space store");
		}

		try
		{
			clear();
			_trackedRevision = revision;

			co::uint32 rootObject = _spaceStore->getRootObject( _trackedRevision );
			restoreObject( rootObject );

			co::IObject* object = static_cast<co::IObject*>( getObject( rootObject ) );

			co::IObject* spaceObj = co::newInstance( "ca.Space" );
			_space = spaceObj->getService<ca::ISpace>();

			spaceObj->setService( "universe", _universe.get() );
			_space->setRootObject( object );
			_space->notifyChanges();
			
			_spaceStore->close();
			
			_space->addSpaceObserver( this );
		}
		catch( ... )
		{
			_spaceStore->close();
			throw;
		}
		
	}

	void save()
	{
		if( _trackedRevision != _spaceStore->getLatestRevision() )
		{
			CORAL_THROW( ca::IOException, "Attempt to save changes in a intermediary revision" );
		}

		co::RefPtr<ca::ISpaceChanges> current;
		for( int i = 0; i < _spaceChanges.size(); i++ )
		{
			current = _spaceChanges[ i ];

			int addedObjectsSize = current->getAddedObjects().getSize();

			for( int j = 0; j < addedObjectsSize; j++ )
			{
				_addedObjects.insert( current->getAddedObjects()[j] );
			}

			Change change;
			ChangeSet changeSet;

			for( int j = 0; j < current->getChangedObjects().getSize(); j++ )
			{
				ca::IObjectChanges * objectChanges = current->getChangedObjects()[j];

				// changes have been made on an added object, there's no need to register these changes.
				// updating the object is enough
				ObjectSet::iterator objIt = _addedObjects.find( objectChanges->getObject() );
				if( objIt != _addedObjects.end() )
				{
					_addedObjects.insert( objIt, objectChanges->getObject() );
					continue;
				}

				ChangeSetCache::iterator it = _changeCache.find( objectChanges->getObject() );

				bool objInserted = ( it != _changeCache.end() );

				changeSet.clear();
				ChangeSet &objChangeSet = ( objInserted ) ? it->second : changeSet;

				for( int k = 0; k < objectChanges->getChangedConnections().getSize(); k++ )
				{
					ca::ChangedConnection changedConn = objectChanges->getChangedConnections()[k];

					change.member = changedConn.receptacle.get();
					change.newValue = changedConn.current.get();
					objChangeSet.insert( change );
				}

				if( !objInserted )
				{
					_changeCache.insert( ChangeSetCache::value_type( objectChanges->getObject(), objChangeSet ) );
				}

				for( int k = 0; k < objectChanges->getChangedServices().getSize(); k++ )
				{
					ca::IServiceChanges* changedServ = objectChanges->getChangedServices()[k];

					ChangeSetCache::iterator it = _changeCache.find( changedServ->getService() );

					bool servInserted = ( it != _changeCache.end() );

					changeSet.clear();
					ChangeSet &serviceChangeSet = ( servInserted ) ? it->second : changeSet;

					for( int l = 0; l < changedServ->getChangedValueFields().getSize(); l++ )
					{
						change.member = changedServ->getChangedValueFields()[l].field.get();
						change.newValue =  changedServ->getChangedValueFields()[l].current;
						serviceChangeSet.insert( change );
					}

					for( int l = 0; l < changedServ->getChangedRefFields().getSize(); l++ )
					{
						change.member = changedServ->getChangedRefFields()[l].field.get();
						change.newValue =  changedServ->getChangedRefFields()[l].current.get();
						serviceChangeSet.insert( change );
					}

					for( int l = 0; l < changedServ->getChangedRefVecFields().getSize(); l++ )
					{
						change.member = changedServ->getChangedRefVecFields()[l].field.get();

						change.newValue.set< const co::RefVector<co::IService>& >( changedServ->getChangedRefVecFields()[l].current );
						serviceChangeSet.insert( change );
					}

					if( !servInserted )
					{
						_changeCache.insert( ChangeSetCache::value_type( changedServ->getService(), serviceChangeSet ) );
					}
				}

				for( int j = 0; j < current->getRemovedObjects().getSize(); j++ )
				{
					co::IObject* currentObject = current->getRemovedObjects()[j];

					// currentObject was removed from the space graph, if it was present on previous changes,
					// these can be ignored

					_addedObjects.erase( currentObject );
					_changeCache.erase( currentObject );

					co::Range<co::IPort* const> ports = currentObject->getComponent()->getPorts();

					for( int k = 0; k < ports.getSize(); k++ )
					{
						_changeCache.erase( currentObject->getServiceAt( ports[k] ) );
					}
				}
			}
		}
		std::vector<ca::StoredFieldValue> values;
		
		_spaceStore->open();
		try
		{
			_spaceStore->beginChanges();

			for( ObjectSet::iterator it = _addedObjects.begin(); it != _addedObjects.end(); it++ )
			{
				saveObject( *it );
			}

			for( ChangeSetCache::iterator it = _changeCache.begin(); it != _changeCache.end(); it++ )
			{
				co::uint32 objectId = getObjectId( it->first );

				for( ChangeSet::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++ )
				{
					ca::StoredFieldValue value;
					value.fieldId = getMemberId( it2->member );
					std::string valueStr;
					if( it2->newValue.getKind() == co::TK_ARRAY )
					{
						co::IType* elementType = it2->newValue.getType();

						if( elementType->getKind() == co::TK_INTERFACE )
						{
							//RefVector value. get database ids;
							const co::RefVector<co::IService>& services = it2->newValue.get<const co::RefVector<co::IService>&>();

							std::vector<co::uint32> serviceIds;
							for( int i = 0; i < services.size(); i++ )
							{
								serviceIds.push_back( getObjectId( services[i].get() ) );
							}

							co::Any serviceIdsAny;
							serviceIdsAny.set< const std::vector< co::uint32 >& >( serviceIds );

							_serializer.toString( serviceIdsAny, valueStr );
						}
						else
						{
							_serializer.toString( it2->newValue, valueStr );
						}
					}
					else if( it2->newValue.getKind() == co::TK_INTERFACE )
					{
						co::IService* service = it2->newValue.get<co::IService*>();
						if( service == NULL)
						{
							valueStr = "nil";
						}
						else
						{
							_serializer.toString( getObjectId( service ), valueStr );
						}
					}
					else
					{
						_serializer.toString( it2->newValue, valueStr );
					}

					value.value = valueStr;
					values.push_back( value );
				}

				_spaceStore->addValues( objectId, values );
				values.clear();
			}
			_spaceStore->commitChanges();
			_spaceStore->close();
			_trackedRevision++;
		}
		catch( ... )
		{
			_spaceStore->discardChanges();
			_spaceStore->close();
			throw;
		}


		_addedObjects.clear();
		_changeCache.clear();
		_spaceChanges.clear();
	}

protected:

	ca::ISpaceStore* getStoreService()
	{
		return _spaceStore.get();
	}

	void setStoreService( ca::ISpaceStore* spaceStore )
	{
		assert( spaceStore );
		_spaceStore = spaceStore;
	}

	ca::IUniverse* getUniverseService()
	{
		return _universe.get();
	}

	void setUniverseService( ca::IUniverse* universe )
	{
		assert( universe );
		_universe = universe;
		_model = static_cast<ca::IModel*>( _universe->getProvider()->getService( "model" ) );
	}

private:

	//save functions

	void savePort( co::uint32 componentId, co::IPort* port )
	{
		co::uint32 typeId = _spaceStore->getOrAddType( port->getType()->getFullName(), getTypeVersionId() );
		
		saveField( port, componentId, typeId, port->getIsFacet() );
	}

	void saveService( co::RefPtr<co::IService> obj, co::IPort* port )
	{
		co::uint32 objId = getObjectId(obj.get());
		if(objId != 0)
		{
			return;
		}

		co::IInterface* type = port->getType();
		saveEntity( type );

		co::uint32 entityId = getTypeId( obj->getInterface() );

		objId = _spaceStore->addObject( entityId );
		insertObjectCache(obj.get(), objId);

		co::Range<co::IField* const> fields = type->getFields();
						
		int j = 0;
		std::vector<ca::StoredFieldValue> values;
		std::string fieldValueStr;
		std::vector<co::int32> refs;
		for( int i = 0; i < fields.getSize(); i++ ) 
		{
			co::IField* field =  fields[i];
			
			co::uint32 fieldId = getMemberId( field );
			if( fieldId == 0 )
			{
				continue;
			}
			co::uint32 fieldTypeId = getTypeId( field->getType() );
			
			co::IReflector* reflector = field->getOwner()->getReflector();
			co::Any fieldValue;

			reflector->getField(obj.get(), field, fieldValue);

			co::TypeKind tk = fieldValue.getKind();

			if( tk == co::TK_ARRAY )
			{
				co::IType* arrayType = static_cast<co::IArray*>( field->getType() )->getElementType();

				if( arrayType->getKind() == co::TK_INTERFACE )
				{
					co::Range<co::IService* const> services = fieldValue.get<co::Range<co::IService* const>>();
					
					refs.clear();
					while( !services.isEmpty() )
					{
						co::IService* const serv = services.getFirst();
						saveObject( serv->getProvider() );
						
						co::uint32 refId = getObjectId( serv );
						
						refs.push_back( refId );
						services.popFirst();
					
					}
					co::Any refsValue;
					refsValue.set<const std::vector<co::int32>&>( refs );
					_serializer.toString( refsValue, fieldValueStr );
				}
				else
				{
					_serializer.toString( fieldValue, fieldValueStr );
				}
				
			}
			else if ( tk == co::TK_INTERFACE )
			{
				co::IService* const service = fieldValue.get<co::IService* const>();
				if( service == NULL )
				{
					fieldValueStr = "nil";
				}
				else
				{
					saveObject( service->getProvider() );
					_serializer.toString( getObjectId(service), fieldValueStr );
				}
				
			}
			else
			{
				_serializer.toString( fieldValue, fieldValueStr );
			}

			ca::StoredFieldValue value;
			value.fieldId = fieldId;
			value.value = fieldValueStr;
			values.push_back( value );
		}
		_spaceStore->addValues( objId, values );
	}

	bool needsToSaveType( co::IType* type )
	{
		return ( getTypeId( type ) == 0 );
	}

	void saveEntity( co::IType* type )
	{
		if( !needsToSaveType( type ) )
		{
			return;
		}

		co::TypeKind tk = type->getKind();

		if( tk == co::TK_COMPONENT )
		{
			co::IComponent*	component = static_cast<co::IComponent*>( type );
			saveComponent( component );
			return;
		}

		if( tk == co::TK_INTERFACE )
		{
			co::IInterface*	interfaceType = static_cast<co::IInterface*>(type);
			saveInterface( interfaceType );
			return;
		}

		co::uint32 typeId = _spaceStore->getOrAddType( type->getFullName(), getTypeVersionId() );
		insertTypeCache( type, typeId );
	}

	void saveField( co::IMember* member, co::uint32 entityId, co::uint32 fieldTypeId, bool isFacet )
	{
		co::uint32 fieldId = _spaceStore->addField( entityId, member->getName(), fieldTypeId, isFacet );
		insertMemberCache( member, fieldId );

	}

	void saveInterface(co::IInterface* entityInterface)
	{

		co::uint32 getIdFromSavedEntity = _spaceStore->getOrAddType( entityInterface->getFullName(), getTypeVersionId() );
		insertTypeCache( entityInterface, getIdFromSavedEntity );

		co::RefVector<co::IField> fields;
		_model->getFields(entityInterface, fields);
		
		for(co::RefVector<co::IField>::iterator it = fields.begin(); it != fields.end(); it++) 
		{
			co::IField* field =  it->get();
			
			co::uint32 fieldTypeId = getTypeId( field->getType() );

			if( fieldTypeId == 0 )
			{
				saveEntity( field->getType() );
				fieldTypeId = getTypeId( field->getType() );
			}
			
			saveField( field, getIdFromSavedEntity, fieldTypeId, false );

		}

	}

	void saveObject(co::RefPtr<co::IObject> object)
	{
		if( getObjectId(object.get()) != 0 )
		{
			return;
		}

		co::IComponent* component = object->getComponent();

		std::string name = component->getFullName();
		saveEntity(component);

		co::uint32 entityId = getTypeId( component );
		co::uint32 objId = _spaceStore->addObject( entityId );

		insertObjectCache( object.get(), objId );

		co::Range<co::IPort* const> ports = component->getPorts();

		co::uint32 fieldId;
		std::vector<ca::StoredFieldValue> values;
		for( int i = 0; i < ports.getSize(); i++ )
		{
			co::IPort* port = (ports[i]);
			fieldId = getMemberId( port );
			
			if( fieldId == 0 )
			{
				continue;
			}
			
			co::IService* service = object->getServiceAt( port );
			
			std::string refStr;
			if(port->getIsFacet())
			{
				saveService(service, port);
				
				_serializer.toString( getObjectId( service ), refStr);
			}
			else
			{
				saveObject( service->getProvider() );
				_serializer.toString( getObjectId( service ), refStr);
			}
			ca::StoredFieldValue value;
			value.fieldId = fieldId;
			value.value = refStr;
			values.push_back( value );
			
		}
		_spaceStore->addValues( objId, values );

	}

	void saveComponent( co::IComponent* component )
	{
		co::uint32 componentId = _spaceStore->getOrAddType( component->getFullName(), getTypeVersionId() );
		insertTypeCache( component, componentId );

		co::RefVector< co::IPort > ports;
		_model->getPorts( component, ports );

		for( int i = 0; i < ports.size(); i++ )
		{
			savePort( componentId, ports[i].get() );
		}
	}

	//restore functions

	void restoreObject( co::uint32 id )
	{
		co::uint32 typeId = _spaceStore->getObjectType( id );

		co::IType* type = getType( typeId );

		const std::string& entity = type->getFullName();
		
		co::IObject* object = co::newInstance( entity );
		
		co::Range<co::IPort* const> ports = object->getComponent()->getPorts();

		std::vector<ca::StoredFieldValue> fieldValues;
		_spaceStore->getValues( id, _trackedRevision, fieldValues );
		
		FieldValueMap mapFieldValue;

		for( int i = 0; i < fieldValues.size(); i++ )
		{
			ca::StoredFieldValue fieldValue = fieldValues[i];
			mapFieldValue.insert( FieldValueMap::value_type( fieldValue.fieldId, fieldValue.value ) );
		}

		insertObjectCache( object, id );

		co::IPort* port;
		for( int i = 0; i < ports.getSize(); i++ )
		{
			port = ports[i];
			
			co::uint32 fieldId = getMemberId( port );
			if( fieldId == 0 )
			{
				continue;
			}

			co::uint32 idService = atoi( mapFieldValue.find(fieldId)->second.c_str() );

			if( port->getIsFacet() )
			{
				co::IService* service = object->getServiceAt( port );
				fillServiceValues( idService, service );
			}
			else 
			{
				co::uint32 idServiceProvider = _spaceStore->getServiceProvider( idService, _trackedRevision );
				co::IObject* refObj = static_cast<co::IObject*>( getObject( idServiceProvider ) );
				if( refObj == NULL )
				{
					throw co::IllegalStateException();
				}
				try
				{
					object->setServiceAt( port, getObject( idService ) );
				}
				catch( co::IllegalCastException& )
				{
					CORAL_THROW( ca::IOException, "Could not restore object, invalid value type for port " << port->getName() << "on entity " << entity );
				}
			}

		}
		
	}

	void fillServiceValues( co::uint32 id, co::IService* service )
	{

		std::vector<ca::StoredFieldValue> fieldValues;

		co::uint32 typeId = _spaceStore->getOrAddType( service->getInterface()->getFullName(), getTypeVersionId() );
		getType( typeId );

		_spaceStore->getValues( id, _trackedRevision, fieldValues );
		FieldValueMap mapFieldValue;

		insertObjectCache( service, id );

		for( int i = 0; i < fieldValues.size(); i++ )
		{
			ca::StoredFieldValue fieldValue = fieldValues[i];
			mapFieldValue.insert( FieldValueMap::value_type( fieldValue.fieldId, fieldValue.value ) );
		}

		co::Range<co::IField* const> fields = service->getInterface()->getFields();
		
		co::IReflector* ref = service->getInterface()->getReflector();
		for( int i = 0; i < fields.getSize(); i++)
		{
			co::IField* field = fields[i];
			co::uint32 fieldId = getMemberId( field );
			if( fieldId == 0 )
			{
				continue;
			}

			fillFieldValue( service, field, id, mapFieldValue.find(fieldId)->second, _trackedRevision );
		}
	}

	void fillFieldValue( co::IService* service, co::IField* field, co::uint32 id, std::string strValue, co::uint32 _trackedRevision )
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
				fillInterfaceArrayValue( strValue, arrayType, fieldValue );
			}
		
		}
		if( type->getKind() == co::TK_INTERFACE )
		{
			if( strValue != "nil" )
			{
				co::Any refId;
				_serializer.fromString( strValue, co::typeOf<co::int32>::get(), refId );
				
				co::uint32 refIdInt = refId.get<co::uint32>();
				co::uint32 providerRefId = _spaceStore->getServiceProvider( refIdInt, _trackedRevision );

				co::IObject* ref = static_cast<co::IObject*>( getObject( providerRefId ) ); //assure the provider was loaded
				if( ref == NULL )
				{
					throw co::IllegalStateException();
				}

				fieldValue.set( getObject( refIdInt ) );
			}

		}
		else
		{
			try
			{
				_serializer.fromString(strValue, field->getType(), fieldValue);
			}
			catch( ca::FormatException& )
			{
				CORAL_THROW( ca::FormatException, "Invalid field value for field " << field->getName() );
			}
			
		}
		
		try
		{
			if( fieldValue.getKind() != co::TK_NONE )
			{
				reflector->setField( serviceAny, field, fieldValue );
			}

		}
		catch ( co::IllegalCastException& )
		{
			CORAL_THROW( ca::IOException, "Invalid field value type for field " << field->getName() << "; type expected: " << field->getType()->getFullName() );
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
			co::uint32 providerId = _spaceStore->getServiceProvider( vec[i], _trackedRevision );
			
			ref = static_cast<co::IObject*>( getObject( providerId ) ); //assure the provider was loaded
			if( ref == NULL )
			{
				throw co::IllegalStateException();
			}
						
			co::Any servicePtr;
			servicePtr.setService( getObject( vec[i] ) );
						
			try
			{
				AnyArrayUtil arrayUtil;
				arrayUtil.setArrayComplexTypeElement( arrayValue, i, servicePtr );
			}
			catch ( co::NotSupportedException& )
			{
				throw ca::IOException( "Could not restore array" );
			}
		}

	}

	co::uint32 getTypeVersionId()
	{
		return 1;
	}

	co::IService* getObject( co::uint32 objectId )
	{
		co::IService* service = getCachedObject( objectId );

		if( service == NULL )
		{
			restoreObject( objectId );
			service = getObject( objectId );
			if(service == NULL)
			{
				throw co::IllegalStateException();
			}
		}
		return service;
	}

	co::IType* getType( co::uint32 id )
	{
		co::IType* resultType = getCachedType( id );

		if( resultType == NULL )
		{
			StoredType storedType;
			_spaceStore->getType( id, storedType );

			resultType = co::getType( storedType.typeName );

			insertTypeCache( resultType, id );
			
			if( resultType->getKind() == co::TK_COMPONENT || resultType->getKind() == co::TK_INTERFACE )
			{
				co::ICompositeType* compositeType = static_cast< co::ICompositeType*>(resultType);
				
				std::string storedFieldName;
				for( int i = 0; i < storedType.fields.size(); i++ )
				{
					storedFieldName = storedType.fields[i].fieldName;
					co::IMember* member = compositeType->getMember( storedFieldName );
					assert( member != NULL );
					insertMemberCache( member, storedType.fields[i].fieldId );
				} 
			}
		}

		return resultType;

	}

	co::uint32 getTypeId( co::IType* type )
	{
		TypeIdMap::iterator it = _typeIdCache.find( type );

		if(it != _typeIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	co::IType* getCachedType( co::uint32 id )
	{
		IdTypeMap::iterator it = _typeCache.find(id);

		if(it != _typeCache.end())
		{
			return it->second;
		}

		return NULL;

	}

	co::IMember* getMember( co::uint32 id )
	{
		IdMemberMap::iterator it = _memberCache.find(id);

		if(it != _memberCache.end())
		{
			return it->second;
		}

		return NULL;

	}

	co::uint32 getMemberId( co::IMember* member )
	{
		MemberIdMap::iterator it = _memberIdCache.find( member );

		if(it != _memberIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	co::IService* getCachedObject( co::uint32 id )
	{
		IdObjectMap::iterator it = _objectCache.find(id);

		if(it != _objectCache.end())
		{
			return it->second;
		}

		return NULL;

	}

	co::uint32 getObjectId(co::IService* obj)
	{
		ObjectIdMap::iterator it = _objectIdCache.find(obj);

		if(it != _objectIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	//generates two way mapping
	void insertObjectCache( co::IService* obj, co::uint32 id )
	{
		_objectIdCache.insert(ObjectIdMap::value_type( obj, id ));
		_objectCache.insert(IdObjectMap::value_type( id, obj ));
	}

	void insertTypeCache( co::IType* type, co::uint32 id )
	{
		_typeIdCache.insert( TypeIdMap::value_type( type, id ) );
		_typeCache.insert( IdTypeMap::value_type( id, type ) );
	}

	void insertMemberCache( co::IMember* member, co::uint32 id )
	{
		_memberIdCache.insert( MemberIdMap::value_type( member, id ) );
		_memberCache.insert( IdMemberMap::value_type( id, member ) );
	}

	void clear()
	{
		_objectCache.clear();
		_objectIdCache.clear();

		_typeCache.clear();
		_typeIdCache.clear();

		_memberCache.clear();
		_memberIdCache.clear();

		if( _space != NULL )
		{
			_space->removeSpaceObserver( this );
		}
	}

private:
	typedef std::map<co::uint32, std::string> FieldValueMap;
	typedef std::map<co::IService*, co::uint32> ObjectIdMap;
	typedef std::map<co::uint32, co::IService*> IdObjectMap;

	typedef std::map<co::IType*, co::uint32> TypeIdMap;
	typedef std::map<co::uint32, co::IType*> IdTypeMap;

	typedef std::map<co::IMember*, co::uint32> MemberIdMap;
	typedef std::map<co::uint32, co::IMember*> IdMemberMap;

	typedef struct Change
	{
	public:
		co::IMember* member;
		co::Any newValue;

		bool operator< ( const Change& change ) const
		{
			return member < change.member;
		}

		bool operator== ( const Change& change ) const
		{
			return member == change.member;
		}

		Change& operator=( const Change& change )
		{
			member = change.member;
			newValue = change.newValue;
		}
	};

	typedef std::set<Change> ChangeSet;
	typedef std::set<co::IObject*> ObjectSet;

	typedef std::map<co::IService*, ChangeSet> ChangeSetCache;

private:

	co::RefPtr<ca::ISpace> _space;
	co::RefPtr<ca::IUniverse> _universe;
	co::RefPtr<ca::ISpaceStore> _spaceStore;

	StringSerializer _serializer;
	ca::IModel* _model;

	co::RefVector<ca::ISpaceChanges> _spaceChanges;

	co::uint32 _trackedRevision;

	ObjectIdMap _objectIdCache;
	IdObjectMap _objectCache;

	TypeIdMap _typeIdCache;
	IdTypeMap _typeCache;

	MemberIdMap _memberIdCache;
	IdMemberMap _memberCache;

	ChangeSetCache _changeCache;
	ObjectSet _addedObjects;

};

CORAL_EXPORT_COMPONENT( SpacePersister, SpacePersister );

} // namespace ca
