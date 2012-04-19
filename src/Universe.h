/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_UNIVERSE_H_
#define _CA_UNIVERSE_H_

#include "Model.h"
#include "GraphChanges.h"
#include "ObjectChanges.h"
#include "Universe_Base.h"

#include <co/Any.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>

#include <ca/ISpace.h>
#include <ca/NotInGraphException.h>

#include <sstream>

#define CHECK_NULL_ARG( name ) \
	if( !name ) throw co::IllegalArgumentException( "illegal null " #name );

namespace ca {

inline void checkComponent( co::IService* service, const char* expectedComponent )
{
	const std::string& name = service->getProvider()->getComponent()->getFullName();
	if( name != expectedComponent )
		CORAL_THROW( co::IllegalArgumentException, "illegal universe instance (expected a "
			<< expectedComponent << ", got a " << name << ")" );
}
	
typedef std::vector<ca::IGraphObserver*> GraphObserverList;

// Data for a graph.
struct GraphRecord
{
	GraphChanges changes;	// list of changes detected in this graph
	bool hasChanges;		// whether the list of 'changes' is non-empty

	GraphObserverList observers;

	GraphRecord() : hasChanges( false )
	{;}

	~GraphRecord()
	{
		// it is highly recommended that all observers unregister themselves
		assert( observers.empty() );
	}
};

// Data for a space.
struct SpaceRecord : GraphRecord
{
	ISpace* space;
	ObjectRecord* rootObject;

	SpaceRecord( ISpace* space ) : space( space ), rootObject( NULL )
	{;}
};

// Data associated with a change section.
struct ChangedService
{
	ObjectRecord* object;	// object that has been changed
	co::int16 facet;		// index of the service that has been changed,
							// or -1 if it was the object's co.IObject facet

	ChangedService( ObjectRecord* object, co::int16 facet )
		: object( object ), facet( facet )
	{;}

	inline bool operator<( const ChangedService& other ) const
	{
		return object < other.object ||
			( object == other.object && facet < other.facet );
	}
};

typedef std::vector<ca::IObjectObserver*> ObjectObserverList;
typedef std::multimap<co::IService*, ca::IServiceObserver*> ServiceObserverMap;
typedef std::pair<ServiceObserverMap::iterator, ServiceObserverMap::iterator> ServiceObserverMapRange;

struct ObjectObservers
{
	ObjectObserverList objectObservers;
	ServiceObserverMap serviceObserverMap;
};

typedef std::map<co::IObject*, ObjectObservers> ObjectObserverMap;

// Data for a universe.
struct UniverseRecord : GraphRecord
{
	co::RefPtr<Model> model;
	std::vector<SpaceRecord*> spaces;

	typedef std::map<co::IObject*, ObjectRecord*> ObjectMap;
	ObjectMap objectMap;

	std::vector<ChangedService> changedServices;

	ObjectObserverMap objectObservers;

	// Finds an object given its component instance. Returns NULL on failure.
	ObjectRecord* findObject( co::IObject* instance )
	{
		ObjectMap::iterator it = objectMap.find( instance );
		return it == objectMap.end() ? NULL : it->second;
	}

	// Gets the record of an object instance. Raises an exception on failure.
	ObjectRecord* getObject( co::IObject* instance )
	{
		ObjectMap::iterator it = objectMap.find( instance );
		if( it == objectMap.end() )
			throw NotInGraphException( "no such object in this graph" );
		return it->second;
	}

	/*
		Creates an object for the given component instance.
		\tparam T either a 'spaceId' (uint16) or an ObjectRecord*.
	 */
	template<typename T>
	ObjectRecord* createObject( T from, co::IObject* instance );

	// Removes from the universe an object that's been removed from all spaces.
	void destroyObject( ObjectRecord* object )
	{		
		ObjectMap::iterator it = objectMap.find( object->instance );
		assert( it != objectMap.end() );
		objectMap.erase( it );
		object->destroy();
	}

	// Accounts for a new reference from a space to a root object.
	void addRef( co::int16 spaceId, ObjectRecord* root );

	// Accounts for a new reference from an object to another in a single space.
	void addRef( ObjectRecord* from, ObjectRecord* to, co::int16 spaceId );

	// Accounts for a new reference from an object to another in all spaces.
	void addRef( ObjectRecord* from, ObjectRecord* to );

	// Accounts for a removed reference from a space to a root object.
	void removeRef( co::int16 spaceId, ObjectRecord* root );

	// Accounts for a removed reference from an object to another in a single space.
	void removeRef( ObjectRecord* from, ObjectRecord* to, co::int16 spaceId );

	// Accounts for a removed reference from an object to another in all spaces.
	void removeRef( ObjectRecord* from, ObjectRecord* to );

	#define ON_CHANGE( EVENT ) \
		hasChanges = true; \
		changes. EVENT ; \
		SpaceRecord* space = spaces[spaceId]; \
		if( !space->observers.empty() ) \
		{ \
			space->hasChanges = true; \
			space->changes. EVENT ; \
		}

	inline void onAddedObject( co::int16 spaceId, ObjectRecord* object )
	{
		ON_CHANGE( addAddedObject( object ) );
	}

	inline void onRemovedObject( co::int16 spaceId, ObjectRecord* object )
	{
		ON_CHANGE( addRemovedObject( object ) );
	}

	inline void onChangedObject( co::int16 spaceId, ObjectChanges* objectChanges )
	{
		ON_CHANGE( addChangedObject( objectChanges ) );
	}

	inline void addChangedService( ObjectRecord* object, co::int16 facet )
	{
		assert( facet >= -1 );
		changedServices.push_back( ChangedService( object, facet ) );
	}
};

class Universe;

class MultiverseObserver
{
public:
	virtual ~MultiverseObserver() {;}
	virtual void onUniverseCreated( Universe* universe ) = 0;
	virtual void onUniverseDestroyed( Universe* universe ) = 0;
};

/*!
	The ca.Universe component.
 */
class Universe : public Universe_Base
{
public:
	inline static void setMultiverseObserver( MultiverseObserver* observer )
	{
		sm_multiverseObserver = observer;
	}

public:
	Universe();
	virtual ~Universe();

	/*!
		Returns whether the given service is being tracked in this universe.
		If the service is being tracked, it's also marked as changed.
	 */
	bool tryAddChange( co::IObject* object, co::IService* service );

	// Methods called from spaces:
	co::int16 spaceRegister( ca::ISpace* space );
	void spaceUnregister( co::int16 spaceId );
	co::IObject* spaceGetRootObject( co::int16 spaceId );
	void spaceInitialize( co::int16 spaceId, co::IObject* root );
	void spaceAddChange( co::int16 spaceId, co::IService* service );
	void spaceAddGraphObserver( co::int16 spaceId, ca::IGraphObserver* observer );
	void spaceRemoveGraphObserver( co::int16 spaceId, ca::IGraphObserver* observer );

	// ca.IGraph methods:
	ca::IModel* getModel();
	void addChange( co::IService* service );
	void notifyChanges();
	void addGraphObserver( ca::IGraphObserver* observer );
	void removeGraphObserver( ca::IGraphObserver* observer );
	void addObjectObserver( co::IObject* object, ca::IObjectObserver* observer );
	void removeObjectObserver( co::IObject* object, ca::IObjectObserver* observer );
	void addServiceObserver( co::IService* service, ca::IServiceObserver* observer );
	void removeServiceObserver( co::IService* service, ca::IServiceObserver* observer );

protected:
	ca::IModel* getModelService();
	void setModelService( ca::IModel* model );

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

	/*!
		Locates the facet index of a service within an object record.
			-1 means the 'co.IObject object' facet.
			-2 means the service is not tracked (i.e. not in the model)
	 */
	inline co::int16 findFacet( ObjectRecord* object, co::IService* service )
	{
		if( service == object->instance )
			return -1;

		co::int16 numFacets = object->model->numFacets;
		for( co::int16 i = 0; i < numFacets; ++i )
			if( service == object->services[i] )
				return i;

		return -2;
	}

private:
	static MultiverseObserver* sm_multiverseObserver;

private:
	UniverseRecord _u;
	co::IService* _lastChangedService;
};

} // namespace ca

#endif // _CA_UNIVERSE_H_
