/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SpacePersister_Base.h"

#include <co/Coral.h>
#include <co/IField.h>
#include <co/IReflector.h>
#include <ca/ISpace.h>
#include <lua/IState.h>

#include <ca/IGraphChanges.h>
#include <ca/IObjectChanges.h>
#include <ca/IServiceChanges.h>
#include <ca/ChangedConnection.h>
#include <ca/ChangedValueField.h>
#include <ca/ChangedRefField.h>
#include <ca/ChangedRefVecField.h>

#include <ca/IOException.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>

#include <map>
#include <set>
#include <deque>

#include "StringSerializer.h"

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
			_spaceStore->close();

		if( _space != NULL && _space->getRootObject() != NULL )
			_space->removeGraphObserver( this );
	}

	// ------ ca.ISpaceObserver Methods ------ //

	void onGraphChanged( ca::IGraphChanges* changes )
	{
		co::TSlice<co::IObject*> addedObjects = changes->getAddedObjects();
		for( ; addedObjects; addedObjects.popFirst() )
			_addedObjects.insert( addedObjects.getFirst() );

		co::TSlice<ca::IObjectChanges*> changedObjects = changes->getChangedObjects();
		for( ; changedObjects; changedObjects.popFirst() )
		{
			ca::IObjectChanges* objectChanges = changedObjects.getFirst();

			// if changes have been made on an added object, there's no need
			// to register these changes (updating the object is enough)
			co::IObject* object = objectChanges->getObject();
			ObjectSet::iterator it = _addedObjects.find( object );
			if( it != _addedObjects.end() )
				continue;

			co::TSlice<ca::ChangedConnection> changedConnections = objectChanges->getChangedConnections();
			if( changedConnections )
			{
				ChangeSet& cs = _changeCache[object];
				for( ; changedConnections; changedConnections.popFirst() )
				{
					const ca::ChangedConnection& cc = changedConnections.getFirst();
					cs[cc.receptacle.get()] = cc.current.get();
				}
			}

			co::TSlice<ca::IServiceChanges*> changedServices = objectChanges->getChangedServices();
			for( ; changedServices; changedServices.popFirst() )
			{
				ca::IServiceChanges* changes = changedServices.getFirst();
				co::IService* service = changes->getService();
				ChangeSet& cs = _changeCache[service];

				co::TSlice<ca::ChangedValueField> changedValues = changes->getChangedValueFields();
				for( ; changedValues; changedValues.popFirst() )
					cs[changedValues.getFirst().field.get()] = changedValues.getFirst().current;

				co::TSlice<ca::ChangedRefField> changedRefs = changes->getChangedRefFields();
				for( ; changedRefs; changedRefs.popFirst() )
					cs[changedRefs.getFirst().field.get()] = changedRefs.getFirst().current;

				co::TSlice<ca::ChangedRefVecField> changedRefVecs = changes->getChangedRefVecFields();
				for( ; changedRefVecs; changedRefVecs.popFirst() )
					cs[changedRefVecs.getFirst().field.get()] = changedRefVecs.getFirst().current;
			}

			co::TSlice<co::IObject*> removedObjects = changes->getRemovedObjects();
			for( ; removedObjects; removedObjects.popFirst() )
			{
				co::IObject* removedObj = removedObjects.getFirst();

				// ignore changes for objects removed from the graph
				_addedObjects.erase( removedObj );
				_changeCache.erase( removedObj );

				co::TSlice<co::IPort*> ports = removedObj->getComponent()->getPorts();
				for( ; ports; ports.popFirst() )
					_changeCache.erase( removedObj->getServiceAt( ports.getFirst() ) );
			}
		}
	}

	// ------ ca.ISpaceLoader Methods ------ //

	void insertObjectCache( co::IService* obj, co::uint32 id )
	{
		_objectIdCache.insert( ObjectIdMap::value_type( obj, id ) );
	}

	void setUpdateList( const std::string& updateList )
	{
		_updateList = updateList;
	}

	void insertNewObject( co::IService* obj )
	{
		_addedObjects.insert( obj );
	}

	void addChange( co::IService* service, co::IMember* member, const co::Any& newValue )
	{
		_changeCache[service][member] = newValue;
	}

	void addRefChange( co::IService* service, co::IMember* member, co::IService* newValue )
	{
		addChange( service, member, newValue );
	}

	void addRefVecChange( co::IService* service, co::IMember* member, std::vector<co::IServiceRef>& newValue )
	{
		addChange( service, member, newValue );
	}

	void addTypeChange( co::IService* service, const std::string& newType )
	{
		// a NULL member signals a type change
		addChange( service, NULL, newType );
	}

	// ------ ca.ISpacePersister Methods ------ //

	ca::ISpace* getSpace()
	{
		return _space.get();
	}

	void initialize( co::IObject* rootObject )
	{
		assertSpaceNotSet();

		if( !_spaceStore.isValid() )
			throw co::IllegalStateException( "space file was not set, could not setup" );

		_spaceStore->open();

		if( _spaceStore->getLatestRevision() > 0 )
			throw ca::IOException( "the space store is currently in use" );

		try
		{
			_spaceStore->beginChanges();
			saveObject( rootObject );
			_spaceStore->setRootObject( getObjectId(rootObject) );

			co::TSlice<std::string> updates = _model->getUpdates();
			std::stringstream updateList;

			for( int i = 0; i < updates.getSize(); ++i )
				updateList << updates[i] << ";";

			_updateList = updateList.str();
			_spaceStore->commitChanges( _updateList );
		}
		catch( ... )
		{
			_spaceStore->discardChanges();
			_spaceStore->close();
			throw;
		}

		co::IObjectRef spaceObj = co::newInstance( "ca.Space" );
		_space = spaceObj->getService<ca::ISpace>();

		spaceObj->setService( "universe", _universe.get() );

		_space->initialize( rootObject );
		_space->notifyChanges();

		_trackedRevision = 1;

		_space->addGraphObserver( this );

		_spaceStore->close();
	}

	void restore()
	{
		checkCanRestore();

		_spaceStore->open();
		co::uint32 latestRevision = _spaceStore->getLatestRevision();
		_spaceStore->close();

		restoreRevision( latestRevision );
	}

	void restoreRevision( co::uint32 revision )
	{
		checkCanRestore();

		_spaceStore->open();

		if( _spaceStore->getLatestRevision() == 0 )
		{
			_spaceStore->close();
			throw co::IllegalArgumentException( "empty space store" );
		}

		_spaceStore->close();

		try
		{
			clear();
			_trackedRevision = revision;
			restoreLua( _trackedRevision );
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
			CORAL_THROW( ca::IOException, "attempt to save changes in an intermediary revision" );

		std::vector<std::string> fieldNames;
		std::vector<std::string> values;
		_spaceStore->open();

		try
		{
			_spaceStore->beginChanges();
			ChangeSet newChangeSet;
			for( ObjectSet::iterator it = _addedObjects.begin(); it != _addedObjects.end(); it++ )
			{
				co::IService* service = *it;
				IObject* object = service->getProvider();

				if( static_cast<IObject*>( service ) == object )
				{
					saveObject( object );
					if( object == _space->getRootObject() )
						_spaceStore->setRootObject( getObjectId( object ) );
				}
				else
				{
					co::IPort* facet = service->getFacet();
					co::uint32 providerId = getObjectId( service->getProvider() );
					saveService( service, facet, providerId );

					addRefChange( service->getProvider(), facet, service );
				}
			}

			for( ChangeSetCache::iterator it = _changeCache.begin(); it != _changeCache.end(); ++it )
			{
				co::uint32 objectId = getObjectId( it->first );
				for( ChangeSet::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2 )
					saveChange( it2->first, it2->second.getAny().asIn(), fieldNames, values );

				_spaceStore->addValues( objectId, fieldNames, values );
				fieldNames.clear();
				values.clear();
			}
			_spaceStore->commitChanges( _updateList );
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
		_serializer.setModel( _model.get() );
	}

private:
	void assertSpaceNotSet()
	{
		if( _space != NULL )
			throw ca::IOException( "space already initialized/restored" );
	}

	void checkCanRestore()
	{
		assertSpaceNotSet();
		if( !_spaceStore.isValid() )
			throw co::IllegalStateException( "space store was not set, can't restore a space" );
	}

	void restoreLua( co::uint32 revision )
	{
		co::IObjectRef spaceObj = co::newInstance( "ca.Space" );
		spaceObj->setService( "universe", _universe.get() );
		_space = spaceObj->getService<ca::ISpace>();

		co::Any args[] = {
			_space.get(),
			_spaceStore.get(),
			_model.get(),
			revision,
			static_cast<ca::ISpaceLoader*>( this )
		};

		co::getService<lua::IState>()->call( "ca.SpaceLoaderFast", "", args, co::Slice<co::Any>() );

		_space->addGraphObserver( this );
		_space->notifyChanges();
	}

	// Save functions


	#define TO_STR( strVar, ostreamStuff ) \
		{ std::stringstream ss; ss << ostreamStuff; \
			ss.str().swap( strVar ); }

	co::uint32 saveService( co::IService* service, co::IPort* port, co::uint32 providerId )
	{
		co::uint32 id = getObjectId( service );
		if( id != 0 )
			return id;

		co::IInterface* type = port->getType();

		id = _spaceStore->addService( type->getFullName(), providerId );
		insertObjectCache( service, id );

		std::vector<co::IFieldRef> fields;
		_model->getFields(type, fields);

		std::vector<std::string> fieldNames;
		std::vector<std::string> values;

		std::string valueStr;

		for( size_t i = 0; i < fields.size(); ++i )
		{
			co::IField* field = fields[i].get();

			co::AnyValue value;
			co::IReflector* reflector = field->getOwner()->getReflector();
			reflector->getField( service, field, value );

			co::TypeKind kind = value.getKind();
			if ( kind == co::TK_INTERFACE )
			{
				co::IService* service = value.get<co::IService*>();
				if( service == NULL )
				{
					valueStr = "nil";
				}
				else
				{
					saveObject( service->getProvider() );
					TO_STR( valueStr, "#" << getObjectId( service ) );
				}
			}
			else if( kind == co::TK_ARRAY &&
				static_cast<co::IArray*>( value.getType() )->
					getElementType()->getKind() == co::TK_INTERFACE  )
			{
				std::vector<co::uint32> refIds;
				co::Slice<co::IService*> refs = value.get<co::Slice<co::IService*> >();
				for( ; refs; refs.popFirst() )
				{
					co::IService* s = refs.getFirst();
					saveObject( s->getProvider() );
					refIds.push_back( getObjectId( s ) );
				}
				TO_STR( valueStr, "#" << refIds );
			}
			else
			{
				_serializer.toString( value.getAny(), valueStr );
			}
			fieldNames.push_back( field->getName() );
			values.push_back( valueStr );
		}

		_spaceStore->addValues( id, fieldNames, values );
		return id;
	}

	co::uint32 saveObject( co::IObject* object )
	{
		co::uint32 id = getObjectId( object );
		if( id != 0 )
			return id;

		co::IComponent* component = object->getComponent();
		id = _spaceStore->addObject( component->getFullName() );
		insertObjectCache( object, id );

		std::vector<std::string> values;
		std::vector<std::string> fieldNames;

		std::vector<co::IPortRef> ports;

		_model->getPorts( component, ports );

		for( size_t i = 0; i < ports.size(); ++i )
		{
			co::IPort* port = ports[i].get();
			co::IService* service = object->getServiceAt( port );

			co::uint32 refId;
			if( port->getIsFacet() )
				refId = saveService( service, port, id );
			else
				refId = saveObject( service->getProvider() );

			std::stringstream ss;
			ss << "#" << refId;
			values.push_back( ss.str() );
			fieldNames.push_back( port->getName() );
		}

		_spaceStore->addValues( id, fieldNames, values );
		return id;
	}

	co::uint32 getObjectId( co::IService* obj )
	{
		ObjectIdMap::iterator it = _objectIdCache.find(obj);
		return it == _objectIdCache.end() ? NULL : it->second;
	}

	void clear()
	{
		_changeCache.clear();
		_addedObjects.clear();
		_objectIdCache.clear();
	}

	void saveChange( co::IMember* member, const co::Any& value,
		std::vector<std::string>& fieldNames,
		std::vector<std::string>& values )
	{
		// a NULL member signals an object type change
		if( member == NULL )
		{
			fieldNames.push_back( "_type" );
			values.push_back( value.get<const std::string&>() );
			return;
		}

		assert( value.isIn() );
		co::TypeKind kind = value.getKind();

		std::string valueStr;
		if( kind == co::TK_INTERFACE )
		{
			co::IService* service = value.get<co::IService*>();
			if( service == NULL )
				valueStr = "nil";
			else
				TO_STR( valueStr, "#" << getObjectId( service ) );
		}
		else if( kind == co::TK_ARRAY && static_cast<co::IArray*>( value.getType() )->getElementType()->getKind() == co::TK_INTERFACE )
		{
			std::vector<co::uint32> refIds;
			co::Slice<co::IService*> refs = value.get<co::Slice<co::IService*> >();
			for( ; refs; refs.popFirst() )
				refIds.push_back( getObjectId( refs.getFirst() ) );
			TO_STR( valueStr, "#" << refIds );
		}
		else 
		{
			_serializer.toString( value, valueStr );
		}
		fieldNames.push_back( member->getName() );
		values.push_back( valueStr );
	}

private:
	typedef std::map<const std::string, std::string> FieldValueMap;
	typedef std::map<co::IService*, co::uint32> ObjectIdMap;

	typedef std::set<co::IService*> ObjectSet;
	typedef std::map<co::IMember*, co::AnyValue> ChangeSet;
	typedef std::map<co::IService*, ChangeSet> ChangeSetCache;

private:
	ca::ISpaceRef _space;
	ca::IUniverseRef _universe;
	ca::ISpaceStoreRef _spaceStore;

	StringSerializer _serializer;
	ca::IModelRef _model;

	co::uint32 _trackedRevision;
	std::string _updateList;

	ObjectIdMap _objectIdCache;

	ChangeSetCache _changeCache;
	ObjectSet _addedObjects;
};

CORAL_EXPORT_COMPONENT( SpacePersister, SpacePersister );

} // namespace ca
