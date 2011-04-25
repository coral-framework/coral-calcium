/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SpaceChanges.h"
#include <algorithm>

namespace ca {

SpaceChanges::SpaceChanges()
{
	// empty
}

SpaceChanges::~SpaceChanges()
{
	// empty
}

inline int objectCompare( const co::IObject* a, const co::IObject* b )
{
	return ( a < b ? -1 : ( a == b ? 0 : 1 ) );
}

inline bool changesStdCompare( const co::RefPtr<ca::IObjectChanges>& a,
							   const co::RefPtr<ca::IObjectChanges>& b )
{
	return static_cast<ObjectChanges*>( a.get() )->getObjectInl()
			< static_cast<ObjectChanges*>( b.get() )->getObjectInl();
}

inline int changesKeyCompare( const co::IObject* key, ca::IObjectChanges* changes )
{
	co::IObject* object = static_cast<ObjectChanges*>( changes )->getObjectInl();
	return ( key == object ? 0 : ( key < object ? -1 : 1 ) );
}

ISpaceChanges* SpaceChanges::finalize( ca::ISpace* space )
{
	assert( !_space.isValid() );

	// sort all arrays
	std::sort( _addedObjects.begin(), _addedObjects.end() );
	std::sort( _removedObjects.begin(), _removedObjects.end() );
	std::sort( _changedObjects.begin(), _changedObjects.end(), changesStdCompare );

	// clone this object
	SpaceChanges* object = new SpaceChanges( *this );
	object->_space = space;

	// reset state
	_addedObjects.clear();
	_removedObjects.clear();
	_changedObjects.clear();

	return object;
}

ca::ISpace* SpaceChanges::getSpace()
{
	return _space.get();
}

co::Range<co::IObject* const> SpaceChanges::getAddedObjects()
{
	return _addedObjects;
}

co::Range<co::IObject* const> SpaceChanges::getRemovedObjects()
{
	return _removedObjects;
}

co::Range<ca::IObjectChanges* const> SpaceChanges::getChangedObjects()
{
	return _changedObjects;
}

bool SpaceChanges::wasAdded( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Range<co::IObject*>( _addedObjects ), object, objectCompare, pos );
}

bool SpaceChanges::wasChanged( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Range<ca::IObjectChanges*>( _changedObjects ), object, changesKeyCompare, pos );
}

bool SpaceChanges::wasRemoved( co::IObject* object )
{
	size_t pos;
	return co::binarySearch( co::Range<co::IObject*>( _removedObjects ), object, objectCompare, pos );
}

CORAL_EXPORT_COMPONENT( SpaceChanges, SpaceChanges );

} // namespace ca
