/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Universe.h"
#include "Universe_Base.h"
#include <co/Any.h>
#include <co/RefPtr.h>
#include <co/IComponent.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>
#include <ca/ModelException.h>
#include <ca/NoSuchObjectException.h>
#include <sstream>

namespace ca {

typedef std::vector<ObjectRecord*> ObjectList;
typedef std::map<co::IObject*, ObjectRecord*> ObjectMap;

/*
	Data for a space within an universe.
 */
struct SpaceRecord
{
	ca::ISpace* space;
	ObjectList rootObjects;

	SpaceRecord( ca::ISpace* space ) : space( space )
	{;}
};

/*
	The ca.Universe component.
 */
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
		co::uint16 numSpaces = static_cast<co::uint16>( _spaces.size() );
		for( co::uint16 i = 0; i < numSpaces; ++i )
		{
			if( _spaces[i] )
			{
				assert( false );
				unregisterSpace( i );
			}
		}

		// at this point the universe should have no objects left
		assert( _objectMap.empty() );
	}

	co::uint16 registerSpace( ca::ISpace* space )
	{
		assert( _spaces.size() < co::MAX_UINT16 );
		co::uint16 spaceId = static_cast<co::uint16>( _spaces.size() );
		_spaces.push_back( new SpaceRecord( space ) );
		return spaceId;
	}

	void unregisterSpace( co::uint16 spaceId )
	{
		SpaceRecord* space = getSpace( spaceId );

		// remove all root objects in the space
		size_t numObjects = space->rootObjects.size();
		for( size_t i = 0; i < numObjects; ++i )
			removeRef( spaceId, space->rootObjects[i] );

		delete space;
		_spaces[spaceId] = NULL;
	}

	void addRootObject( co::uint16 spaceId, co::IObject* root )
	{
		checkHasModel();

		SpaceRecord* space = getSpace( spaceId );
		ObjectRecord* object = findObject( root );
		if( object )
			addRef( spaceId, object );
		else
			object = createObject( spaceId, root );
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
				removeRef( spaceId, rootObjects[i] );
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

		ObjectRecord* object = getObject( service->getProvider() );
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

protected:
	inline void checkHasModel()
	{
		if( !_model.isValid() )
			throw co::IllegalStateException( "the ca.Universe requires a model for this operation" );
	}

	ca::IModel* getModelService()
	{
		return _model.get();
	}

	void setModelService( ca::IModel* model )
	{
		if( _model.isValid() )
			throw co::IllegalStateException( "once set, the model of a ca.Universe cannot be changed" );

		if( !model )
			throw co::IllegalArgumentException( "illegal null model" );

		if( model->getProvider()->getComponent()->getFullName() != "ca.Model" )
			CORAL_THROW( co::IllegalArgumentException,
				"illegal model instance (expected a ca.Model, got a "
				<< model->getProvider()->getComponent()->getFullName() << ")" );

		_model = static_cast<Model*>( model );
	}

private:
	inline Model* getModel()
	{
		if( !_model.isValid() )
			throw co::IllegalStateException( "" );
		return _model.get();
	}

	inline SpaceRecord* getSpace( co::uint16 spaceId )
	{
		return _spaces[spaceId];
	}

	// Finds an object given its component instance. Returns NULL on failure.
	ObjectRecord* findObject( co::IObject* instance )
	{
		ObjectMap::iterator it = _objectMap.find( instance );
		return it == _objectMap.end() ? NULL : it->second;
	}

	// Gets an object given its component instance. Raises an exception on failure.
	ObjectRecord* getObject( co::IObject* instance )
	{
		ObjectMap::iterator it = _objectMap.find( instance );
		if( it == _objectMap.end() )
			throw ca::NoSuchObjectException( "no such object in this space" );
		return it->second;
	}

	// Creates an object for the given component instance.
	template<typename T>
	ObjectRecord* createObject( T from, co::IObject* instance )
	{
		// instantiate and register the object
		ComponentRecord* model = _model->getComponent( instance->getComponent() );
		ObjectRecord* object = ObjectRecord::create( model, instance );
		_objectMap.insert( ObjectMap::value_type( instance, object ) );

		addRef( from, object );

		UpdateTraverser traverser( this );
		traverseObject( object, traverser );

		return object;
	}

	void destroyObject( ObjectRecord* object )
	{		
		ObjectMap::iterator it = _objectMap.find( object->instance );
		assert( it != _objectMap.end() );
		_objectMap.erase( it );
		object->destroy();
	}

	struct UpdateTraverser
	{
		Universe* self;
		UpdateTraverser( Universe* self ) : self( self ) {;}

		void initRef( ObjectRecord* from, co::IService*& svc, ObjectRecord*& to, co::IService* newService )
		{
			svc = newService;
			co::IObject* newInstance = newService->getProvider();
			ObjectRecord* newTo = self->findObject( newInstance );
			if( newTo )
				self->addRef( from, newTo );
			else
				newTo = self->createObject( from, newInstance );
			to = newTo;
		}

		void updateRef( ObjectRecord* from, co::IService*& svc, ObjectRecord*& to, co::IService* newService )
		{
			if( svc == newService )
				return;

			svc = newService;

			co::IObject* newInstance = newService->getProvider();
			ObjectRecord* newTo = self->findObject( newInstance );
			if( to != newTo )
			{
				if( newTo )
					self->addRef( from, newTo );
				else
					newTo = self->createObject( from, newInstance );
				if( to )
					self->removeRef( from, to );
				to = newTo;
			}
		}

		void operator()( ObjectRecord* object, PortRecord& receptacle, RefField& ref )
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

	struct AddRefTraverser
	{
		Universe* self;
		co::uint16 spaceId;
		AddRefTraverser( Universe* self, co::uint16 spaceId )
			: self( self ), spaceId( spaceId )
		{;}

		void operator()( ObjectRecord* object, PortRecord& receptacle, RefField& ref )
		{
			self->addRef( object, ref.object, spaceId );
		}

		void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefField& ref )
		{
			self->addRef( object, ref.object, spaceId );
		}

		void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
		{
			size_t size = refVec.getSize();
			for( size_t i = 0; i < size; ++i )
				self->addRef( object, refVec.objects[i], spaceId );
		}
	};

	struct RemoveRefTraverser
	{
		Universe* self;
		co::uint16 spaceId;
		RemoveRefTraverser( Universe* self, co::uint16 spaceId )
			: self( self ), spaceId( spaceId )
		{;}

		void operator()( ObjectRecord* object, PortRecord& receptacle, RefField& ref )
		{
			self->removeRef( object, ref.object, spaceId );
		}

		void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefField& ref )
		{
			self->removeRef( object, ref.object, spaceId );
		}

		void operator()( ObjectRecord* object, co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
		{
			size_t size = refVec.getSize();
			for( size_t i = 0; i < size; ++i )
				self->removeRef( object, refVec.objects[i], spaceId );
		}
	};

	// Accounts for a new reference from a space to a root object.
	void addRef( co::uint16 spaceId, ObjectRecord* root )
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

	// Accounts for a removed reference from a space to a root object.
	void removeRef( co::uint16 spaceId, ObjectRecord* root )
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

	// Accounts for a new reference from an object to another.
	void addRef( ObjectRecord* from, ObjectRecord* to, co::uint16 spaceId )
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
	
	// Accounts for a removed reference from an object to another.
	void removeRef( ObjectRecord* from, ObjectRecord* to, co::uint16 spaceId )
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

	// Accounts for a new reference from an object to another.
	void addRef( ObjectRecord* from, ObjectRecord* to )
	{
		assert( from && to );

		// increment to's ref-count for each of from's spaces
		SpaceRefCountMap::iterator end = from->spaceRefs.end();
		for( SpaceRefCountMap::iterator it = from->spaceRefs.begin(); it != end; ++it )
			addRef( from, to, it->first );
	}

	// Accounts for a removed reference from an object to another.
	void removeRef( ObjectRecord* from, ObjectRecord* to )
	{
		assert( from && to );

		// decrement to's ref-count for each of from's spaces
		SpaceRefCountMap::iterator end = from->spaceRefs.end();
		for( SpaceRefCountMap::iterator it = from->spaceRefs.begin(); it != end; ++it )
			removeRef( from, to, it->first );
	}

private:
	co::RefPtr<Model> _model;
	std::vector<SpaceRecord*> _spaces;
	ObjectMap _objectMap;
};

CORAL_EXPORT_COMPONENT( Universe, Universe )

} // namespace ca
