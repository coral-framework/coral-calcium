/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ObjectChanges_Base.h"
#include <ca/IReceptacleChange.h>
#include <co/IObject.h>
#include <ca/IServiceChanges.h>

namespace ca {

class ObjectChanges : public ObjectChanges_Base
{
public:
	ObjectChanges()
	{
		// empty constructor
	}

	virtual ~ObjectChanges()
	{
		// empty destructor
	}

	// ------ ca.IObjectChanges Methods ------ //

	co::Range<ca::IReceptacleChange* const> getChangedReceptacles()
	{
		// TODO: implement this method.
		return co::Range<ca::IReceptacleChange* const>();
	}

	co::Range<ca::IServiceChanges* const> getChangedServices()
	{
		// TODO: implement this method.
		return co::Range<ca::IServiceChanges* const>();
	}

	co::IObject* getObject()
	{
		// TODO: implement this method.
		return NULL;
	}

private:
	// member variables go here
};

CORAL_EXPORT_COMPONENT( ObjectChanges, ObjectChanges );

} // namespace ca
