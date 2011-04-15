/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Universe.h"
#include "Universe_Base.h"
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>
#include <ca/ModelException.h>
#include <sstream>

namespace ca {

//------ InitTraverser ---------------------------------------------------------

struct InitTraverser
{
	UniverseRecord* _u;

	InitTraverser( UniverseRecord* u ) : _u( u ) {;}

	void initRef( ObjectRecord* from, co::IService*& service, ObjectRecord*& to, co::IService* newService )
	{
		service = newService;
		co::IObject* newInstance = newService->getProvider();
		ObjectRecord* newTo = _u->findObject( newInstance );
		if( newTo )
			_u->addRef( from, newTo );
		else
			newTo = _u->createObject( from, newInstance );
		to = newTo;
	}

	inline void operator()( ObjectRecord* object, PortRecord& receptacle, RefField& ref )
	{
		assert( !ref.service && !ref.object );
		initRef( object, ref.service, ref.object, object->instance->getService( receptacle.port ) );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		assert( !ref.service && !ref.object );

		co::Any any;
		field.getOwnerReflector()->getField( object->services[facetId], field.field, any );
		assert( any.getKind() == co::TK_INTERFACE );

		initRef( object, ref.service, ref.object, any.getState().data.service );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		assert( !refVec.services && !refVec.objects );

		co::Any any;
		field.getOwnerReflector()->getField( object->services[facetId], field.field, any );
		co::Range<co::IService* const> range = any.get<co::Range<co::IService* const> >();

		size_t size = range.getSize();
		refVec.create( size );
		for( size_t i = 0; i < size; ++i )
			initRef( object, refVec.services[i], refVec.objects[i], range[i] );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, void* valuePtr )
	{
		co::Any any;
		field.getOwnerReflector()->getField( object->services[facetId], field.field, any );

		co::TypeKind kind = any.getKind();
		bool isPrimitive = ( kind >= co::TK_BOOLEAN && kind <= co::TK_DOUBLE || kind == co::TK_ENUM );
		const void* fromPtr = ( isPrimitive ? &any.getState().data : any.getState().data.ptr );
		field.getTypeReflector()->copyValue( fromPtr, valuePtr );
	}
};

template<typename T>
ObjectRecord* UniverseRecord::createObject( T from, co::IObject* instance )
{
	// instantiate and register the object
	ComponentRecord* component = model->getComponent( instance->getComponent() );
	ObjectRecord* object = ObjectRecord::create( component, instance );
	objectMap.insert( ObjectMap::value_type( instance, object ) );

	addRef( from, object );

	InitTraverser traverser( this );
	traverseObject( object, traverser );

	return object;
}

//------ UpdateTraverser -------------------------------------------------------

struct UpdateTraverser
{
	UniverseRecord* _u;

	UpdateTraverser( UniverseRecord* u ) : _u( u ) {;}

	void updateRef( ObjectRecord* from, co::IService*& service, ObjectRecord*& to, co::IService* newService )
	{
		if( service == newService )
			return;

		service = newService;

		co::IObject* newInstance = newService->getProvider();
		ObjectRecord* newTo = _u->findObject( newInstance );
		if( to != newTo )
		{
			if( newTo )
				_u->addRef( from, newTo );
			else
				newTo = _u->createObject( from, newInstance );
			if( to )
				_u->removeRef( from, to );
			to = newTo;
		}
	}

	void operator()( ObjectRecord* object, PortRecord& receptacle, RefField& ref )
	{
		assert( !ref.service && !ref.object );
		updateRef( object, ref.service, ref.object, object->instance->getService( receptacle.port ) );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		assert( !ref.service && !ref.object );

		co::Any any;
		field.getOwnerReflector()->getField( object->services[facetId], field.field, any );
		assert( any.getKind() == co::TK_INTERFACE );

		updateRef( object, ref.service, ref.object, any.getState().data.service );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		assert( !refVec.services && !refVec.objects );

		co::Any any;
		field.getOwnerReflector()->getField( object->services[facetId], field.field, any );
		co::Range<co::IService* const> range = any.get<co::Range<co::IService* const> >();

		size_t size = range.getSize();
		refVec.create( size );
		for( size_t i = 0; i < size; ++i )
			updateRef( object, refVec.services[i], refVec.objects[i], range[i] );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, void* valuePtr )
	{
		co::Any any;
		field.getOwnerReflector()->getField( object->services[facetId], field.field, any );

		co::TypeKind kind = any.getKind();
		bool isPrimitive = ( kind >= co::TK_BOOLEAN && kind <= co::TK_DOUBLE || kind == co::TK_ENUM );
		const void* fromPtr = ( isPrimitive ? &any.getState().data : any.getState().data.ptr );
		field.getTypeReflector()->copyValue( fromPtr, valuePtr );
	}
};

//------ AddRefTraverser -------------------------------------------------------

struct AddRefTraverser
{
	UniverseRecord* _u;
	co::uint16 spaceId;

	AddRefTraverser( UniverseRecord* u, co::uint16 spaceId ) : _u( u ), spaceId( spaceId )
	{;}

	void operator()( ObjectRecord* object, PortRecord& receptacle, RefField& ref )
	{
		_u->addRef( object, ref.object, spaceId );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		_u->addRef( object, ref.object, spaceId );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		size_t size = refVec.getSize();
		for( size_t i = 0; i < size; ++i )
			_u->addRef( object, refVec.objects[i], spaceId );
	}
};

void UniverseRecord::addRef( co::uint16 spaceId, ObjectRecord* root )
{
	assert( root );

	++root->inDegree;
	if( ++root->spaceRefs[spaceId] > 1 )
		return; // object was already in this space

	// this object has just been added to a new space...

	// recursively update ref-counts for the new space
	if( root->outDegree )
	{
		AddRefTraverser traverser( this, spaceId );
		traverseObjectRefs( root, traverser );
	}
}

void UniverseRecord::addRef( ObjectRecord* from, ObjectRecord* to, co::uint16 spaceId )
{
	assert( from && to );

	++from->outDegree;
	++to->inDegree;
	if( ++to->spaceRefs[spaceId] == 1 )
	{
		// 'to' has just been added to a new space...

		// recursively update ref-counts for the new space
		if( to->outDegree )
		{
			AddRefTraverser traverser( this, spaceId );
			traverseObjectRefs( to, traverser );
		}
	}
}

void UniverseRecord::addRef( ObjectRecord* from, ObjectRecord* to )
{
	assert( from && to );

	// increment to's ref-count for each of from's spaces
	SpaceRefCountMap::iterator end = from->spaceRefs.end();
	for( SpaceRefCountMap::iterator it = from->spaceRefs.begin(); it != end; ++it )
		addRef( from, to, it->first );
}

//------ RemoveRefTraverser ----------------------------------------------------

struct RemoveRefTraverser
{
	UniverseRecord* _u;
	co::uint16 spaceId;

	RemoveRefTraverser( UniverseRecord* u, co::uint16 spaceId ) : _u( u ), spaceId( spaceId )
	{;}

	void operator()( ObjectRecord* object, PortRecord& receptacle, RefField& ref )
	{
		_u->removeRef( object, ref.object, spaceId );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		_u->removeRef( object, ref.object, spaceId );
	}

	void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		size_t size = refVec.getSize();
		for( size_t i = 0; i < size; ++i )
			_u->removeRef( object, refVec.objects[i], spaceId );
	}
};

void UniverseRecord::removeRef( co::uint16 spaceId, ObjectRecord* root )
{
	assert( root );

	--root->inDegree;
	if( --root->spaceRefs[spaceId] > 0 )
		return;

	// object was removed from space

	// propagate removal from space
	if( root->outDegree )
	{
		RemoveRefTraverser traverser( this, spaceId );
		traverseObjectRefs( root, traverser );
	}

	// if this was the last space, destroy the object
	if( root->inDegree == 0 )
		destroyObject( root );
}

void UniverseRecord::removeRef( ObjectRecord* from, ObjectRecord* to, co::uint16 spaceId )
{
	assert( from && to );

	--from->outDegree;
	--to->inDegree;
	if( --to->spaceRefs[spaceId] == 0 )
	{
		// 'to' was removed from space

		// propagate removal from this space
		if( to->outDegree )
		{
			RemoveRefTraverser traverser( this, spaceId );
			traverseObjectRefs( to, traverser );
		}

		// if this was to's last space, destroy it
		if( to->inDegree == 0 )
			destroyObject( to );
	}
}

void UniverseRecord::removeRef( ObjectRecord* from, ObjectRecord* to )
{
	assert( from && to );

	// decrement to's ref-count for each of from's spaces
	SpaceRefCountMap::iterator end = from->spaceRefs.end();
	for( SpaceRefCountMap::iterator it = from->spaceRefs.begin(); it != end; ++it )
		removeRef( from, to, it->first );
}

//------ ca.Universe component -------------------------------------------------

class Universe : public Universe_Base
{
public:
	Universe()
	{
		// empty
	}

	virtual ~Universe()
	{
		// all spaces should have been unregistered by now
		co::uint16 numSpaces = static_cast<co::uint16>( _u.spaces.size() );
		for( co::uint16 i = 0; i < numSpaces; ++i )
		{
			if( _u.spaces[i] )
			{
				assert( false );
				unregisterSpace( i );
			}
		}

		// at this point the universe should have no objects left
		assert( _u.objectMap.empty() );
	}

	co::uint16 registerSpace( ca::ISpace* space )
	{
		assert( _u.spaces.size() < co::MAX_UINT16 );
		co::uint16 spaceId = static_cast<co::uint16>( _u.spaces.size() );
		_u.spaces.push_back( new SpaceRecord( space ) );
		return spaceId;
	}

	void unregisterSpace( co::uint16 spaceId )
	{
		SpaceRecord* space = getSpace( spaceId );

		// remove all root objects in the space
		size_t numObjects = space->rootObjects.size();
		for( size_t i = 0; i < numObjects; ++i )
			_u.removeRef( spaceId, space->rootObjects[i] );

		delete space;
		_u.spaces[spaceId] = NULL;
	}

	void addRootObject( co::uint16 spaceId, co::IObject* root )
	{
		checkHasModel();

		SpaceRecord* space = getSpace( spaceId );
		ObjectRecord* object = _u.findObject( root );
		if( object )
			_u.addRef( spaceId, object );
		else
			object = _u.createObject( spaceId, root );
		space->rootObjects.push_back( object );
	}

	void removeRootObject( co::uint16 spaceId, co::IObject* root )
	{
		ObjectList& rootObjects = getSpace( spaceId )->rootObjects;
		size_t numObjects = rootObjects.size();
		for( size_t i = 0; i < numObjects; ++i )
		{
			if( rootObjects[i]->instance == root )
			{
				_u.removeRef( spaceId, rootObjects[i] );
				rootObjects.erase( rootObjects.begin() + i );
				return;
			}
		}
		throw ca::NoSuchObjectException( "no such root object in this space" );
	}

	void getRootObjects( co::uint16 spaceId, co::RefVector<co::IObject>& result )
	{
		ObjectList& rootObjects = getSpace( spaceId )->rootObjects;
		size_t numObjects = rootObjects.size();
		for( size_t i = 0; i < numObjects; ++i )
			result.push_back( rootObjects[i]->instance );
	}

	co::uint32 beginChange( co::uint16 spaceId, co::IService* service )
	{
		checkHasModel();

		if( !service )
			throw ca::NoSuchObjectException( "illegal null service" );

		ObjectRecord* object = _u.getObject( service->getProvider() );
		if( !object )
			throw ca::NoSuchObjectException( "service is not provided by an object in this universe" );

		#ifndef CORAL_NDEBUG
			// for debug builds, we also check if the object is in the space
			if( object->spaceRefs[spaceId] < 1 )
				throw ca::NoSuchObjectException( "service is not provided by an object in this space" );
		#else
			CORAL_UNUSED( spaceId );
		#endif

		// locate the service's facet
		co::uint8 numFacets = object->model->numFacets;
		co::uint8 facetId = 0;
		while( 1 )
		{
			if( facetId >= numFacets )
				throw ca::ModelException( "illegal service: facet is not in the object model" );

			if( service == object->services[facetId] )
				break;

			++facetId;
		}

		// TODO
		return 0;
	}

	void endChange( co::uint16 spaceId, co::uint32 level )
	{
		// TODO
		CORAL_UNUSED( spaceId );
		CORAL_UNUSED( level );
	}

	void notifyChanges()
	{
		// TODO
	}

protected:
	inline void checkHasModel()
	{
		if( !_u.model.isValid() )
			throw co::IllegalStateException( "the ca.Universe requires a model for this operation" );
	}

	ca::IModel* getModelService()
	{
		return _u.model.get();
	}

	void setModelService( ca::IModel* model )
	{
		if( _u.model.isValid() )
			throw co::IllegalStateException( "once set, the model of a ca.Universe cannot be changed" );

		if( !model )
			throw co::IllegalArgumentException( "illegal null model" );

		if( model->getProvider()->getComponent()->getFullName() != "ca.Model" )
			CORAL_THROW( co::IllegalArgumentException,
				"illegal model instance (expected a ca.Model, got a "
				<< model->getProvider()->getComponent()->getFullName() << ")" );

		_u.model = static_cast<Model*>( model );
	}

private:
	inline SpaceRecord* getSpace( co::uint16 spaceId )
	{
		return _u.spaces[spaceId];
	}

private:
	UniverseRecord _u;
};

CORAL_EXPORT_COMPONENT( Universe, Universe )

} // namespace ca
