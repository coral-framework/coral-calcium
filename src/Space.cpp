/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Space_Base.h"
#include <ca/IUniverse.h>
#include <co/RefPtr.h>
#include <co/IllegalStateException.h>

namespace ca {

class Space : public Space_Base
{
public:
	Space()
	{
		_spaceId = -1;
	}

	virtual ~Space()
	{
		if( _universe.isValid() )
			_universe->unregisterSpace( _spaceId );
	}

	void addRootObject( co::IObject* root )
	{
		checkRegistered();
		_universe->addRootObject( _spaceId, root );
	}

	void removeRootObject( co::IObject* root )
	{
		checkRegistered();
		_universe->removeRootObject( _spaceId, root );
	}
	
	void getRootObjects( co::RefVector<co::IObject>& result )
	{
		checkRegistered();
		_universe->getRootObjects( _spaceId, result );
	}

	void beginChange( co::IService* facet )
	{
		checkRegistered();
		_universe->beginChange( facet );
	}

	void endChange( co::IService* facet )
	{
		checkRegistered();
		_universe->endChange( facet );
	}

protected:
	inline void checkRegistered()
	{
		if( !_universe.isValid() )
			throw co::IllegalStateException( "the ca.Space requires a universe for this operation" );
	}

	ca::IUniverse* getUniverseService()
	{
		return _universe.get();
	}

	void setUniverseService( ca::IUniverse* universe )
	{
		if( _universe.isValid() )
			throw co::IllegalStateException( "once set, the universe of a ca.Space cannot be changed" );

		_spaceId = universe->registerSpace( this );
		_universe = universe;
	}

private:
	co::RefPtr<ca::IUniverse> _universe;
	co::int16 _spaceId;
};
	
CORAL_EXPORT_COMPONENT( Space, Space )

} // namespace ca
