/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Universe.h"
#include "Universe_Base.h"
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>
#include <ca/ModelException.h>
#include <ca/UnexpectedException.h>
#include <sstream>

namespace ca {

//------ Utility Macros for Handling Exceptions --------------------------------

#define GEN_BARRIER( code, facetId, msg ) try { code; } catch( std::exception& e ) { \
	const char* name = getServiceTypeName( source, facetId ); \
	CORAL_THROW( ca::UnexpectedException, \
		"unexpected exception while tracking changes to service (" << \
		name << ")" << source->instance << ", raised by " << msg ); }

#define OBJ_BARRIER( code ) \
	GEN_BARRIER( code, -1, "receptacle '" << receptacle.port->getName() << "'" )

#define SVC_BARRIER( code ) \
	GEN_BARRIER( code, facetId, "field '" << field.field->getName() << "'" )

//------ UniverseTraverser (base for most traversers) --------------------------

template<typename ConcreteType>
struct UniverseTraverser : public Traverser<ConcreteType>
{
	typedef UniverseTraverser<ConcreteType> UT;

	UniverseRecord& u;

	UniverseTraverser( UniverseRecord& u, ObjectRecord* source )
		: Traverser<ConcreteType>( source ), u( u )
	{;}

	void initRef( co::IService*& service, ObjectRecord*& target, co::IService* newService )
	{
		service = newService;

		ObjectRecord* newTarget = NULL;
		if( newService )
		{
			co::IObject* newInstance = newService->getProvider();
			newTarget = u.findObject( newInstance );
			if( newTarget )
				u.addRef( this->source, newTarget );
			else
				newTarget = u.createObject( this->source, newInstance );
		}

		target = newTarget;
	}

	void updateRef( co::IService*& service, ObjectRecord*& target, co::IService* newService )
	{
		assert( service != newService );
		service = newService;

		ObjectRecord* newTarget = NULL;
		if( newService )
		{		
			co::IObject* newInstance = newService->getProvider();
			newTarget = u.findObject( newInstance );
			if( newTarget )
			{
				if( target != newTarget )
					u.addRef( this->source, newTarget );
			}
			else
			{
				newTarget = u.createObject( this->source, newInstance );
			}
		}

		if( target != newTarget )
		{
			if( target )
				u.removeRef( this->source, target );
			
			target = newTarget;
		}
	}
};

//------ InitTraverser (initializes a new object) ------------------------------

struct InitTraverser : public UniverseTraverser<InitTraverser>
{
	InitTraverser( UniverseRecord& u, ObjectRecord* source ) : UT( u, source )
	{;}

	void onReceptacle( PortRecord& receptacle, RefField& ref )
	{
		assert( !ref.service && !ref.object );
		co::IService* newService;
		OBJ_BARRIER( newService = source->instance->getService( receptacle.port ) );
		initRef( ref.service, ref.object, newService );
	}

	void onRefField( co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		assert( !ref.service && !ref.object );

		co::Any any;
		SVC_BARRIER( field.getOwnerReflector()->getField( source->services[facetId], field.field, any ) );
		assert( any.getKind() == co::TK_INTERFACE );

		initRef( ref.service, ref.object, any.getState().data.service );
	}

	void onRefVecField( co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		assert( !refVec.services && !refVec.objects );

		co::Any any;
		SVC_BARRIER( field.getOwnerReflector()->getField( source->services[facetId], field.field, any ) );
		co::Range<co::IService* const> range = any.get<co::Range<co::IService* const> >();

		size_t size = range.getSize();
		if( size )
		{
			refVec.create( size );
			for( size_t i = 0; i < size; ++i )
				initRef( refVec.services[i], refVec.objects[i], range[i] );
		}
	}

	void onValueField( co::uint8 facetId, FieldRecord& field, void* valuePtr )
	{
		co::Any any;
		SVC_BARRIER( field.getOwnerReflector()->getField( source->services[facetId], field.field, any ) );

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

	InitTraverser traverser( *this, object );
	try
	{
		traverser.traverseObject();
	}
	catch( ... )
	{
		removeRef( from, object );
		throw;
	}

	return object;
}

//------ UpdateTraverser (updates an existing object) --------------------------

struct UpdateTraverser : public UniverseTraverser<UpdateTraverser>
{
	ObjectChanges* objectChanges;
	ServiceChanges* serviceChanges;
	co::int16 lastFacet;

	UpdateTraverser( UniverseRecord& u ) : UT( u, NULL ),
		objectChanges( NULL ), serviceChanges( NULL ), lastFacet( -1 )
	{;}

	~UpdateTraverser()
	{
		clear();
	}

	void clear()
	{
		if( !objectChanges )
		{
			co::debug( co::Dbg_Warning, "ca.Universe: addChange() called for (%s)%p without changes.",
				source->model->type->getFullName().c_str(), source->instance );
			return;
		}

		// add the 'changes' to each of this object's spaces
		SpaceRefCountMap::iterator end = source->spaceRefs.end();
		for( SpaceRefCountMap::iterator it = source->spaceRefs.begin(); it != end; ++it )
			u.onChangedObject( it->first, objectChanges );

		lastFacet = -1;
		objectChanges = NULL;
		serviceChanges = NULL;
	}

	void reset( ObjectRecord* newSource )
	{
		assert( source != newSource );
		if( source )
			clear();
		source = newSource;
	}

	ObjectChanges* getObjectChanges()
	{
		if( !objectChanges )
			objectChanges = new ObjectChanges( source->instance );
		return objectChanges;
	}

	ServiceChanges* getServiceChanges( co::uint8 facetId )
	{
		if( lastFacet != facetId )
		{
			serviceChanges = new ServiceChanges( source->services[facetId] );
			getObjectChanges()->addChangedService( serviceChanges );
			lastFacet = facetId;
		}
		return serviceChanges;
	}

	void onReceptacle( PortRecord& receptacle, RefField& ref )
	{
		co::IService* service;
		OBJ_BARRIER( service = source->instance->getService( receptacle.port ) );
		if( service == ref.service )
			return; // no change

		ChangedConnection& cc = getObjectChanges()->addChangedConnection();
		cc.receptacle = receptacle.port;
		cc.previous = ref.service;
		cc.current = service;

		updateRef( ref.service, ref.object, service );
	}

	void onRefField( co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		co::Any any;
		SVC_BARRIER( field.getOwnerReflector()->getField( source->services[facetId], field.field, any ) );
		assert( any.getKind() == co::TK_INTERFACE );

		co::IService* service = any.getState().data.service;
		if( service == ref.service )
			return; // no change

		ChangedRefField& cf = getServiceChanges( facetId )->addChangedRefField();
		cf.field = field.field;
		cf.previous = ref.service;
		cf.current = service;

		updateRef( ref.service, ref.object, service );
	}

	void onRefVecField( co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		co::Any any;
		SVC_BARRIER( field.getOwnerReflector()->getField( source->services[facetId], field.field, any ) );
		co::Range<co::IService* const> range = any.get<co::Range<co::IService* const> >();

		size_t newSize = range.getSize();
		size_t oldSize = refVec.getSize();
		if( newSize == oldSize && ( !newSize ||
				std::equal( &range.getFirst(), &range.getLast(), refVec.services ) ) )
			return; // no change

		ChangedRefVecField& cf = getServiceChanges( facetId )->addChangedRefVecField();
		cf.field = field.field;
	
		// populate the 'previous' RefVector
		cf.previous.resize( oldSize );
		for( size_t i = 0; i < oldSize; ++i )
			cf.previous[i] = refVec.services[i];

		// populate the 'current' RefVector
		cf.current.resize( newSize );
		for( size_t i = 0; i < newSize; ++i )
			cf.current[i] = range[i];

		// create a new RefVec
		RefVecField newRefVec;
		newRefVec.create( newSize );
		for( size_t i = 0; i < newSize; ++i )
			initRef( newRefVec.services[i], newRefVec.objects[i], range[i] );

		// destroy the old RefVec
		for( size_t i = 0; i < oldSize; ++i )
			u.removeRef( source, refVec.objects[i] );

		refVec.destroy();
		refVec = newRefVec;
	}

	template<typename ET>
	static inline bool isEqualAligned( void* a, void* b, size_t len )
	{
		for( size_t i = 0; i < len; ++i )
			if( reinterpret_cast<ET*>( a )[i] != reinterpret_cast<ET*>( b )[i] )
				return false;
		return true;
	}

	static inline bool isEqual( void* a, void* b, size_t len )
	{
		return ( ( len % sizeof(size_t) ) == 0 ) ?
			isEqualAligned<size_t>( a, b, len / sizeof(size_t) ) :
			isEqualAligned<co::uint8>( a, b, len );
	}

	static inline bool isEqualStr( void* a, void* b )
	{
		return *reinterpret_cast<std::string*>( a ) == *reinterpret_cast<std::string*>( b );
	}

	void onValueField( co::uint8 facetId, FieldRecord& field, void* valuePtr )
	{
		co::Any any;
		SVC_BARRIER( field.getOwnerReflector()->getField( source->services[facetId], field.field, any ) );

		co::IType* type = field.field->getType();
		co::IReflector* reflector = type->getReflector();

		// perform raw memory comparison
		co::Any::State& s = any.getState();
		bool isPrimitive = ( s.kind <= co::TK_DOUBLE || s.kind == co::TK_ENUM );
		assert( !s.isPointer && ( ( isPrimitive && !s.isReference ) || ( !isPrimitive && s.isReference ) ) );
		void* newValuePtr = ( isPrimitive ? &s.data : s.data.ptr );
		if( ( s.kind == co::TK_STRING && isEqualStr( newValuePtr, valuePtr ) ) ||
			isEqual( newValuePtr, valuePtr, reflector->getSize() ) )
			return; // no change

		ChangedValueField& cf = getServiceChanges( facetId )->addChangedValueField();
		cf.field = field.field;

		if( isPrimitive )
		{
			cf.previous.setVariable( type, co::Any::VarIsValue, valuePtr );
			cf.current = any;
		}
		else
		{
			reflector->copyValue( valuePtr, cf.previous.createComplexValue( type ) );
			reflector->copyValue( newValuePtr, cf.current.createComplexValue( type ) );
		}

		// update our internal value
		reflector->copyValue( newValuePtr, valuePtr );
	}
};

//------ AddRefTraverser -------------------------------------------------------

struct AddRefTraverser : public UniverseTraverser<AddRefTraverser>
{
	co::uint16 spaceId;

	AddRefTraverser( UniverseRecord& u, co::uint16 spaceId, ObjectRecord* source )
		: UT( u, source ), spaceId( spaceId )
	{;}

	void onReceptacle( PortRecord& receptacle, RefField& ref )
	{
		u.addRef( source, ref.object, spaceId );
	}

	void onRefField( co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		u.addRef( source, ref.object, spaceId );
	}

	void onRefVecField( co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		size_t size = refVec.getSize();
		for( size_t i = 0; i < size; ++i )
			u.addRef( source, refVec.objects[i], spaceId );
	}
};

void UniverseRecord::addRef( co::uint16 spaceId, ObjectRecord* root )
{
	assert( root );

	++root->inDegree;
	if( ++root->spaceRefs[spaceId] > 1 )
		return; // object was already in this space

	// this object has just been added to a new space...
	onAddedObject( spaceId, root );

	// recursively update ref-counts for the new space
	if( root->outDegree )
	{
		AddRefTraverser traverser( *this, spaceId, root );
		traverser.traverseObjectRefs();
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
		onAddedObject( spaceId, to );

		// recursively update ref-counts for the new space
		if( to->outDegree )
		{
			AddRefTraverser traverser( *this, spaceId, to );
			traverser.traverseObjectRefs();
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

struct RemoveRefTraverser : public UniverseTraverser<RemoveRefTraverser>
{
	co::uint16 spaceId;

	RemoveRefTraverser( UniverseRecord& u, co::uint16 spaceId, ObjectRecord* source )
		: UT( u, source ), spaceId( spaceId )
	{;}

	void onReceptacle( PortRecord& receptacle, RefField& ref )
	{
		u.removeRef( source, ref.object, spaceId );
	}

	void onRefField( co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		u.removeRef( source, ref.object, spaceId );
	}

	void onRefVecField( co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		size_t size = refVec.getSize();
		for( size_t i = 0; i < size; ++i )
			u.removeRef( source, refVec.objects[i], spaceId );
	}
};

void UniverseRecord::removeRef( co::uint16 spaceId, ObjectRecord* root )
{
	assert( root );

	--root->inDegree;
	if( --root->spaceRefs[spaceId] > 0 )
		return;

	// object was removed from space
	onRemovedObject( spaceId, root );

	// propagate removal from space
	if( root->outDegree )
	{
		RemoveRefTraverser traverser( *this, spaceId, root );
		traverser.traverseObjectRefs();
	}

	// if this was the last space, destroy the object
	if( root->inDegree == 0 )
		destroyObject( root );
}

void UniverseRecord::removeRef( ObjectRecord* from, ObjectRecord* to, co::uint16 spaceId )
{
	assert( from );
	if( !to ) return; // null reference

	--from->outDegree;
	--to->inDegree;
	if( --to->spaceRefs[spaceId] == 0 )
	{
		// 'to' was removed from space
		onRemovedObject( spaceId, to );

		// propagate removal from this space
		if( to->outDegree )
		{
			RemoveRefTraverser traverser( *this, spaceId, to );
			traverser.traverseObjectRefs();
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

		if( space->rootObject )
			_u.removeRef( spaceId, space->rootObject );

		delete space;
		_u.spaces[spaceId] = NULL;
	}

	void setRootObject( co::uint16 spaceId, co::IObject* root )
	{
		checkHasModel();

		SpaceRecord* space = getSpace( spaceId );

		if( space->rootObject )
			throw co::IllegalStateException( "a root object was already set for this space" );

#if 0
		// if we ever want to allow the root object to be 'reset':
		if( root == NULL )
		{
			_u.removeRef( spaceId, space->rootObject );
			space->rootObject = NULL;
			return;
		}
#endif

		ObjectRecord* object = _u.findObject( root );
		if( object )
			_u.addRef( spaceId, object );
		else
			object = _u.createObject( spaceId, root );

		space->rootObject = object;
	}

	co::IObject* getRootObject( co::uint16 spaceId )
	{
		ObjectRecord* object = getSpace( spaceId )->rootObject;
		return object ? object->instance : NULL;
	}

	void addChange( co::uint16 spaceId, co::IService* service )
	{
		checkHasModel();

		if( !service )
			throw ca::NoSuchObjectException( "illegal null service" );

		ObjectRecord* object = _u.getObject( service->getProvider() );
		if( !object )
			throw ca::NoSuchObjectException(
				"service is not provided by an object in this space (nor universe)" );

		if( object->spaceRefs[spaceId] < 1 )
			throw ca::NoSuchObjectException( "service is not provided by an object in this space" );

		// locate the service's facet
		co::int16 facet = -1; // -1 means the object's co.IObject facet
		if( object->instance != service )
		{
			co::int16 numFacets = object->model->numFacets;
			while( 1 )
			{
				if( facet >= numFacets )
					throw ca::ModelException( "the service's facet is not in the object model" );

				if( service == object->services[facet] )
					break;

				++facet;
			}
		}

		_u.addChangedService( object, facet );
	}

	void notifyChanges( co::uint16 spaceId )
	{
		// process the list of changed services
		if( !_u.changedServices.empty() )
		{
			ChangedService* cs = &_u.changedServices.front();
			ChangedService* lastCS = &_u.changedServices.back();
			std::sort( cs, lastCS + 1 );

			UpdateTraverser traverser( _u );
			try
			{
				for( ; cs <= lastCS ; ++cs )
				{
					if( cs->object != traverser.source )
						traverser.reset( cs->object );
					else if( cs->facet == traverser.lastFacet )
						continue;

					if( cs->facet < 0 )
						traverser.traverseReceptacles();
					else
						traverser.traverseFacet( static_cast<co::uint8>( cs->facet ) );
				}
			}
			catch( std::exception& e )
			{
				CORAL_THROW( ca::UnexpectedException,
					"unexpected exception while tracking changes to service (" <<
						getServiceTypeName( cs->object, cs->facet ) << ")" <<
							cs->object->instance << ", " << e.what() );
			}

			_u.changedServices.clear();
		}

		// notify all spaces with changes
		size_t numSpaces = _u.spaces.size();
		for( size_t i = 0; i < numSpaces; ++i )
		{
			SpaceRecord* s = _u.spaces[i];
			if( s->hasChanges )
			{
				co::RefPtr<ISpaceChanges> changes( s->changes.finalize( s->space ) );
				s->space->onSpaceChanged( changes.get() );
				s->hasChanges = false;
			}
		}
	}

private:
	inline SpaceRecord* getSpace( co::uint16 spaceId )
	{
		return _u.spaces[spaceId];
	}

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
	UniverseRecord _u;
};

CORAL_EXPORT_COMPONENT( Universe, Universe )

} // namespace ca
