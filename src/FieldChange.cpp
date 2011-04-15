/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "FieldChange_Base.h"
#include <co/IField.h>

namespace ca {

class FieldChange : public FieldChange_Base
{
public:
	FieldChange()
	{
		// empty constructor
	}

	virtual ~FieldChange()
	{
		// empty destructor
	}

	// ------ ca.IFieldChange Methods ------ //

	const co::Any& getCurrent()
	{
		// TODO: implement this method.
		static co::Any dummy;
		return dummy;
	}

	co::IField* getField()
	{
		// TODO: implement this method.
		return NULL;
	}

	const co::Any& getPrevious()
	{
		// TODO: implement this method.
		static co::Any dummy;
		return dummy;
	}

private:
	// member variables go here
};

CORAL_EXPORT_COMPONENT( FieldChange, FieldChange );

} // namespace ca
