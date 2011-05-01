/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_UNIVERSE_H_
#define _CA_UNIVERSE_H_

#include "Model.h"
#include "SpaceChanges.h"
#include "ObjectChanges.h"

#include <ca/ISpace.h>
#include <ca/NoSuchObjectException.h>

#include <co/Any.h>
#include <co/RefPtr.h>
#include <co/IComponent.h>

namespace ca {

typedef std::vector<ObjectRecord*> ObjectList;

// Data for a space within a calcium universe.
struct SpaceRecord
{
	ISpace* space;			// the space's facade service
	ObjectList rootObjects;	// list of root objects in the space
	SpaceChanges changes;	// list of changes detected in the space
	bool hasChanges;		// whether the list of 'changes' is non-empty

	SpaceRecord( ISpace* space ) : space( space ), hasChanges( false )
	{;}
};

// Data associated with a change section.
struct ChangedService
{
	ObjectRecord* object;	// object that has been changed
	co::int16 facet;	// index of the service that has been changed,
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

// Data for a calcium universe.
struct UniverseRecord
{
	co::RefPtr<Model> model;
	std::vector<SpaceRecord*> spaces;

	typedef std::map<co::IObject*, ObjectRecord*> ObjectMap;
	ObjectMap objectMap;

	std::vector<ChangedService> changedServices;

	// Finds an object given its component instance. Returns NULL on failure.
	ObjectRecord* findObject( co::IObject* instance )
	{
		ObjectMap::iterator it = objectMap.find( instance );
		return it == objectMap.end() ? NULL : it->second;
	}

	// Gets an object given its component instance. Raises an exception on failure.
	ObjectRecord* getObject( co::IObject* instance )
	{
		ObjectMap::iterator it = objectMap.find( instance );
		if( it == objectMap.end() )
			throw NoSuchObjectException( "no such object in this space" );
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
	void addRef( co::uint16 spaceId, ObjectRecord* root );

	// Accounts for a new reference from an object to another in a single space.
	void addRef( ObjectRecord* from, ObjectRecord* to, co::uint16 spaceId );

	// Accounts for a new reference from an object to another in all spaces.
	void addRef( ObjectRecord* from, ObjectRecord* to );

	// Accounts for a removed reference from a space to a root object.
	void removeRef( co::uint16 spaceId, ObjectRecord* root );

	// Accounts for a removed reference from an object to another in a single space.
	void removeRef( ObjectRecord* from, ObjectRecord* to, co::uint16 spaceId );

	// Accounts for a removed reference from an object to another in all spaces.
	void removeRef( ObjectRecord* from, ObjectRecord* to );

	inline void onAddedObject( co::uint16 spaceId, ObjectRecord* object )
	{
		SpaceRecord* space = spaces[spaceId];
		space->hasChanges = true;
		space->changes.addAddedObject( object );
	}

	inline void onRemovedObject( co::uint16 spaceId, ObjectRecord* object )
	{
		SpaceRecord* space = spaces[spaceId];
		space->hasChanges = true;
		space->changes.addRemovedObject( object );
	}

	inline void onChangedObject( co::uint16 spaceId, ObjectChanges* objectChanges )
	{
		SpaceRecord* space = spaces[spaceId];
		space->hasChanges = true;
		space->changes.addChangedObject( objectChanges );
	}

	inline void addChangedService( ObjectRecord* object, co::int16 facet )
	{
		assert( facet >= -1 );
		changedServices.push_back( ChangedService( object, facet ) );
	}
};

} // namespace ca

#endif // _CA_UNIVERSE_H_
