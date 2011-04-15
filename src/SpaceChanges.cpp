/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SpaceChanges_Base.h"
#include <ca/IFieldChange.h>

namespace ca {

class SpaceChanges : public SpaceChanges_Base
{
public:
	SpaceChanges()
	{
		// empty constructor
	}

	virtual ~SpaceChanges()
	{
		// empty destructor
	}

	// ------ ca.IServiceChanges Methods ------ //

	co::Range<ca::IFieldChange* const> getChangedFields()
	{
		// TODO: implement this method.
		return co::Range<ca::IFieldChange* const>();
	}

	co::IService* getService()
	{
		// TODO: implement this method.
		return NULL;
	}

private:
	// member variables go here
};

CORAL_EXPORT_COMPONENT( SpaceChanges, SpaceChanges );

} // namespace ca
