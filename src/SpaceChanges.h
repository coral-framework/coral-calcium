/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_SPACECHANGES_H_
#define _CA_SPACECHANGES_H_

#include "Model.h"
#include "ObjectChanges.h"
#include "SpaceChanges_Base.h"
#include <co/IObject.h>
#include <co/RefVector.h>
#include <ca/ISpace.h>
#include <ca/IObjectChanges.h>

namespace ca {

class SpaceChanges : public SpaceChanges_Base
{
public:
	SpaceChanges();
	virtual ~SpaceChanges();

	// ------ Internal Methods ------ //

	void addAddedObject( ObjectRecord* object )
	{
		_addedObjects.push_back( object->instance );
	}

	void addRemovedObject( ObjectRecord* object )
	{
		_removedObjects.push_back( object->instance );
	}

	void addChangedObject( ObjectChanges* objectChanges )
	{
		_changedObjects.push_back( objectChanges );
	}

	/*
		Prepares this changeset for dissemination. This creates an immutable
		clone of this object, with 'space' set, and then resets this object.
	 */
	ISpaceChanges* finalize( ca::ISpace* space );

	// ------ ca.ISpaceChanges Methods ------ //

	ca::ISpace* getSpace();
	co::Range<co::IObject* const> getAddedObjects();	
	co::Range<co::IObject* const> getRemovedObjects();
	co::Range<ca::IObjectChanges* const> getChangedObjects();
	bool wasAdded( co::IObject* object );
	bool wasChanged( co::IObject* object );
	bool wasRemoved( co::IObject* object );

private:
	co::RefPtr<ca::ISpace> _space;
	co::RefVector<co::IObject> _addedObjects;
	co::RefVector<co::IObject> _removedObjects;
	co::RefVector<ca::IObjectChanges> _changedObjects;
};

} // namespace ca

#endif
