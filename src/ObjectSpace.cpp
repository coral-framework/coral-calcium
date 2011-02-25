/*
 * Calcium Object Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ObjectSpace_Base.h"

namespace ca {

class ObjectSpace : public ObjectSpace_Base
{
public:
	ObjectSpace()
	{
	}

	virtual ~ObjectSpace()
	{
	}

	co::ArrayRange<co::Component* const> getRootObjects()
	{
		return co::ArrayRange<co::Component* const>();
	}

	void addRootObject( co::Interface* root )
	{
	}

	void beginChange( co::Interface* object )
	{
	}

	void endChange()
	{
	}

	void removeRootObject( co::Interface* root )
	{
	}

	void reset()
	{
	}

private:
};

} // namespace ca

CORAL_EXPORT_COMPONENT( ca::ObjectSpace, ObjectSpace )
