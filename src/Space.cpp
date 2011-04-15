/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Space_Base.h"
#include <ca/IUniverse.h>
#include <co/RefPtr.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>

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

	co::uint32 beginChange( co::IService* facet )
	{
		checkRegistered();
		return _universe->beginChange( _spaceId, facet );
	}

	void endChange( co::uint32 level )
	{
		checkRegistered();
		_universe->endChange( _spaceId, level );
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

		if( !universe )
			throw co::IllegalArgumentException( "illegal null universe" );

		_spaceId = universe->registerSpace( this );
		_universe = universe;
	}

private:
	co::RefPtr<ca::IUniverse> _universe;
	co::int16 _spaceId;
};
	
CORAL_EXPORT_COMPONENT( Space, Space )

} // namespace ca
