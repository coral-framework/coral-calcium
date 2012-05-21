/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Universe.h"
#include <co/Log.h>
#include <ca/ModelException.h>
#include <ca/IGraphObserver.h>
#include <ca/IObjectObserver.h>
#include <ca/IServiceObserver.h>
#include <ca/UnexpectedException.h>

namespace ca {

//------ Utility Macros for Handling Exceptions --------------------------------

#define GEN_BARRIER( code, facetId, msg ) try { code; } catch( std::exception& ) { \
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
		OBJ_BARRIER( newService = source->instance->getServiceAt( receptacle.port ) );
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
		bool isPrimitive = ( ( kind >= co::TK_BOOLEAN && kind <= co::TK_DOUBLE ) || kind == co::TK_ENUM );
		const void* fromPtr = ( isPrimitive ? &any.getState().data : any.getState().data.ptr );
		field.getTypeReflector()->copyValues( fromPtr, valuePtr, 1 );
	}
};

template<typename T>
ObjectRecord* UniverseRecord::createObject( T from, co::IObject* instance )
{
	// instantiate and register the object
	ComponentRecord* component = model->getComponentRec( instance->getComponent() );
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
			return;

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
		OBJ_BARRIER( service = source->instance->getServiceAt( receptacle.port ) );
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
			reflector->copyValues( valuePtr, cf.previous.createComplexValue( type ), 1 );
			reflector->copyValues( newValuePtr, cf.current.createComplexValue( type ), 1 );
		}

		// update our internal value
		reflector->copyValues( newValuePtr, valuePtr, 1 );
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

void UniverseRecord::addRef( co::int16 spaceId, ObjectRecord* root )
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

void UniverseRecord::addRef( ObjectRecord* from, ObjectRecord* to, co::int16 spaceId )
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

void UniverseRecord::removeRef( co::int16 spaceId, ObjectRecord* root )
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

void UniverseRecord::removeRef( ObjectRecord* from, ObjectRecord* to, co::int16 spaceId )
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

/******************************************************************************/
/* Helper Functions for Handling Observers                                    */
/******************************************************************************/

template<typename Observer>
inline void checkObserverRemoved( const char* prefix, Observer* observer, size_t numRemoved )
{
	if( numRemoved == 0 )
		throw co::IllegalArgumentException( "no such observer in the list" );

	if( numRemoved > 1 )
	{
		const char* typeName = "null";
		if( observer )
			typeName = observer->getProvider()->getComponent()->getFullName().c_str();

		CORAL_LOG(WARNING) << prefix << " (" << typeName << ')' <<
			observer << " was removed " << numRemoved << " times.";
	}
}

template<typename Container, typename Observer>
inline void removeObserver( const char* prefix, Container& c, Observer* observer )
{
	typename Container::iterator end = c.end();
	typename Container::iterator newEnd = std::remove( c.begin(), end, observer );
	size_t numRemoved = std::distance( newEnd, end );
	c.erase( newEnd, end );
	checkObserverRemoved( prefix, observer, numRemoved );
}

template<typename Container, typename Iterator, typename Observer>
inline void removeObserver( const char* prefix, Container& c, Iterator begin, Iterator end, Observer* observer )
{
	int numRemoved = 0;
	while( begin != end )
	{
		if( begin->second == observer )
		{
			c.erase( begin++ );
			++numRemoved;
		}
		else
		{
			++begin;
		}
	}
	checkObserverRemoved( prefix, observer, numRemoved );
}

void raiseUnexpected( const char* desc, co::IService* service, co::Exception& e )
{
	co::IObject* obj = service->getProvider();
	CORAL_THROW( UnexpectedException, "unexpected " << e.getTypeName()
		<< " raised by " << desc << " (" << obj->getComponent()->getFullName()
		<< ")" << obj << ": " << e.getMessage() );
}


/******************************************************************************/
/* ca.Universe component                                                      */
/******************************************************************************/

MultiverseObserver* Universe::sm_multiverseObserver;

Universe::Universe()
{
	_lastChangedService = NULL;

	if( sm_multiverseObserver )
		sm_multiverseObserver->onUniverseCreated( this );
}

Universe::~Universe()
{
	if( sm_multiverseObserver )
		sm_multiverseObserver->onUniverseDestroyed( this );

	// all spaces should have been unregistered by now
	co::uint16 numSpaces = static_cast<co::uint16>( _u.spaces.size() );
	for( co::uint16 i = 0; i < numSpaces; ++i )
	{
		if( _u.spaces[i] )
		{
			assert( false );
			spaceUnregister( i );
		}
	}

	// at this point the universe should have no objects left
	assert( _u.objectMap.empty() );
}

bool Universe::tryAddChange( co::IObject* object, co::IService* service )
{
	if( service == _lastChangedService )
		return true;

	ObjectRecord* objRec = _u.findObject( object );
	if( objRec )
	{
		co::int16 facet = findFacet( objRec, service );
		if( facet > -2 )
		{
			_lastChangedService = service;
			_u.addChangedService( objRec, facet );
			return true;
		}
	}
	return false;
}

co::int16 Universe::spaceRegister( ca::ISpace* space )
{
	assert( _u.spaces.size() < co::MAX_INT16 );
	co::int16 spaceId = static_cast<co::int16>( _u.spaces.size() );
	_u.spaces.push_back( new SpaceRecord( space ) );
	return spaceId;
}

void Universe::spaceUnregister( co::int16 spaceId )
{
	SpaceRecord* space = getSpace( spaceId );

	if( space->rootObject )
		_u.removeRef( spaceId, space->rootObject );

	delete space;
	_u.spaces[spaceId] = NULL;
}

co::IObject* Universe::spaceGetRootObject( co::int16 spaceId )
{
	ObjectRecord* object = getSpace( spaceId )->rootObject;
	return object ? object->instance : NULL;
}

void Universe::spaceInitialize( co::int16 spaceId, co::IObject* root )
{
	checkHasModel();

	SpaceRecord* space = getSpace( spaceId );

	if( space->rootObject )
		throw co::IllegalStateException( "space was already initialized" );

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

void Universe::spaceAddChange( co::int16 spaceId, co::IService* service )
{
	if( service == _lastChangedService && service )
		return;

	checkHasModel();

	if( !service )
		throw NotInGraphException( "illegal null service" );

	ObjectRecord* object = _u.getObject( service->getProvider() );
	if( !object )
		throw NotInGraphException(
			"service is not provided by an object in this space (nor universe)" );

	if( spaceId >= 0 && object->spaceRefs[spaceId] < 1 )
		throw NotInGraphException( "service is not provided by an object in this space" );

	co::int16 facet = findFacet( object, service );
	if( facet == -2 )
		throw NotInGraphException( "the service's facet is not in the object model" );

	_u.addChangedService( object, facet );
}

void Universe::spaceAddGraphObserver( co::int16 spaceId, ca::IGraphObserver* observer )
{
	CHECK_NULL_ARG( observer );
	getSpace( spaceId )->observers.push_back( observer );
}

void Universe::spaceRemoveGraphObserver( co::int16 spaceId, ca::IGraphObserver* observer )
{
	removeObserver( "graph observer", getSpace( spaceId )->observers, observer );
}

ca::IModel* Universe::getModel()
{
	return _u.model.get();
}

void Universe::addChange( co::IService* service )
{
	spaceAddChange( -1, service );
}

void notifyObjectObservers( ObjectObserverMap& observers, co::Range<ca::IObjectChanges* const> changes )
{
	for( ; changes; changes.popFirst() )
	{
		ca::IObjectChanges* objectChanges = changes.getFirst();
		co::IObject* object = objectChanges->getObject();
		ObjectObserverMap::iterator it = observers.find( object );
		if( it == observers.end() )
			continue;

		// notify object observers
		ca::IObjectObserver* objectObserver;
		try
		{
			ObjectObserverList& ool = it->second.objectObservers;
			size_t numObjectObservers = ool.size();
			for( size_t i = 0; i < numObjectObservers; ++i )
			{
				objectObserver = ool[i];
				objectObserver->onObjectChanged( objectChanges );
			}
		}
		catch( co::Exception& e )
		{
			raiseUnexpected( "object observer", objectObserver, e );
		}
		
		// notify service observers
		ca::IServiceObserver* serviceObserver;
		try
		{
			co::Range<IServiceChanges* const> changedServices = objectChanges->getChangedServices();
			for( ; changedServices; changedServices.popFirst() )
			{
				ca::IServiceChanges* serviceChanges = changedServices.getFirst();
				co::IService* service = serviceChanges->getService();
				ServiceObserverMapRange range = it->second.serviceObserverMap.equal_range( service );
				for( ; range.first != range.second; ++range.first )
				{
					serviceObserver = range.first->second;
					serviceObserver->onServiceChanged( serviceChanges );
				}
			}
		}
		catch( co::Exception& e )
		{
			raiseUnexpected( "service observer", serviceObserver, e );
		}
	}
}

void notifyGraphObservers( GraphRecord* graph, IGraphChanges* changes )
{
	if( !graph->hasChanges )
		return;

	graph->hasChanges = false;

	ca::IGraphObserver* current = NULL;
	try
	{
		size_t numObservers = graph->observers.size();
		for( size_t i = 0; i < numObservers; ++i )
		{
			current = graph->observers[i];
			current->onGraphChanged( changes );
		}
	}
	catch( co::Exception& e )
	{
		raiseUnexpected( "graph observer", current, e );
	}
}

void Universe::notifyChanges()
{
	_lastChangedService = NULL;

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

	// notify observers...

	if( !_u.hasChanges )
		return;

	co::RefPtr<IGraphChanges> changes( _u.changes.finalize( this ) );
	notifyObjectObservers( _u.objectObservers, changes->getChangedObjects() );
	notifyGraphObservers( &_u, changes.get() );

	size_t numSpaces = _u.spaces.size();
	for( size_t i = 0; i < numSpaces; ++i )
	{
		SpaceRecord* space = _u.spaces[i];
		changes = space->changes.finalize( space->space );
		notifyGraphObservers( space, changes.get() );
	}
}

void Universe::addGraphObserver( ca::IGraphObserver* observer )
{
	CHECK_NULL_ARG( observer );
	_u.observers.push_back( observer );
}

void Universe::removeGraphObserver( ca::IGraphObserver* observer )
{
	removeObserver( "graph observer", _u.observers, observer );
}

void Universe::addObjectObserver( co::IObject* object, ca::IObjectObserver* observer )
{
	CHECK_NULL_ARG( object );
	CHECK_NULL_ARG( observer );
	_u.objectObservers[object].objectObservers.push_back( observer );
}

void Universe::removeObjectObserver( co::IObject* object, ca::IObjectObserver* observer )
{
	ObjectObserverMap::iterator it = _u.objectObservers.find( object );
	if( it == _u.objectObservers.end() )
		throw co::IllegalArgumentException( "object is not being observed" );

	ObjectObserverList& ool = it->second.objectObservers;
	removeObserver( "object observer", ool, observer );

	if( ool.empty() && it->second.serviceObserverMap.empty() )
		_u.objectObservers.erase( it );
}

void Universe::addServiceObserver( co::IService* service, ca::IServiceObserver* observer )
{
	CHECK_NULL_ARG( service );
	CHECK_NULL_ARG( observer );

	co::IObject* provider = service->getProvider();
	if( service == provider )
		throw co::IllegalArgumentException( "illegal service type; for services "
			"of type 'co.IObject' please call addObjectObserver() instead" );

	_u.objectObservers[provider].serviceObserverMap.insert(
		ServiceObserverMap::value_type( service, observer ) );
}

void Universe::removeServiceObserver( co::IService* service, ca::IServiceObserver* observer )
{
	CHECK_NULL_ARG( service );
	ObjectObserverMap::iterator it = _u.objectObservers.find( service->getProvider() );
	if( it == _u.objectObservers.end() )
		throw co::IllegalArgumentException( "service is not being observed" );

	ServiceObserverMap& som = it->second.serviceObserverMap;
	ServiceObserverMapRange range = som.equal_range( service );
	removeObserver( "service observer", som, range.first, range.second, observer );

	if( som.empty() && it->second.objectObservers.empty() )
		_u.objectObservers.erase( it );
}

ca::IModel* Universe::getModelService()
{
	return _u.model.get();
}

void Universe::setModelService( ca::IModel* model )
{
	if( _u.model.isValid() )
		throw co::IllegalStateException( "once set, the model of a ca.Universe cannot be changed" );

	CHECK_NULL_ARG( model );
	checkComponent( model, "ca.Model" );

	_u.model = static_cast<Model*>( model );
}

CORAL_EXPORT_COMPONENT( Universe, Universe )

} // namespace ca
