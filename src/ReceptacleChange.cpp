/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ReceptacleChange_Base.h"
#include <co/IPort.h>

namespace ca {

class ReceptacleChange : public ReceptacleChange_Base
{
public:
	ReceptacleChange()
	{
		// empty constructor
	}

	virtual ~ReceptacleChange()
	{
		// empty destructor
	}

	// ------ ca.IReceptacleChange Methods ------ //

	co::IService* getCurrent()
	{
		// TODO: implement this method.
		return NULL;
	}

	co::IService* getPrevious()
	{
		// TODO: implement this method.
		return NULL;
	}

	co::IPort* getReceptacle()
	{
		// TODO: implement this method.
		return NULL;
	}

private:
	// member variables go here
};

CORAL_EXPORT_COMPONENT( ReceptacleChange, ReceptacleChange );

} // namespace ca
