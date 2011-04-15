/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_UNIVERSE_H_
#define _CA_UNIVERSE_H_

#include "Model.h"

#include <ca/ISpace.h>
#include <ca/NoSuchObjectException.h>

#include <co/Any.h>
#include <co/RefPtr.h>
#include <co/IComponent.h>

namespace ca {

typedef std::vector<ObjectRecord*> ObjectList;
typedef std::map<co::IObject*, ObjectRecord*> ObjectMap;

// Data for a space within a calcium universe.
struct SpaceRecord
{
	ISpace* space;
	ObjectList rootObjects;

	SpaceRecord( ISpace* space ) : space( space )
	{;}
};

// Data for a calcium universe.
struct UniverseRecord
{
	co::RefPtr<Model> model;
	std::vector<SpaceRecord*> spaces;
	ObjectMap objectMap;

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
};

} // namespace ca

#endif // _CA_UNIVERSE_H_
