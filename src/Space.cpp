/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Universe.h"
#include "Space_Base.h"
#include <ca/IGraphChanges.h>
#include <ca/IObjectChanges.h>
#include <ca/IServiceChanges.h>
#include <co/Log.h>
#include <co/Coral.h>
#include <algorithm>
#include <map>

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
			_universe->spaceUnregister( _spaceId );
	}

	ca::IModel* getModel()
	{
		checkRegistered();
		return _universe->getModel();
	}

	ca::IUniverse* getUniverse()
	{
		return _universe.get();
	}

	co::IObject* getRootObject()
	{
		checkRegistered();
		return _universe->spaceGetRootObject( _spaceId );
	}

	void initialize( co::IObject* root )
	{
		checkRegistered();
		_universe->spaceInitialize( _spaceId, root );
	}

	void addChange( co::IService* facet )
	{
		checkRegistered();
		return _universe->spaceAddChange( _spaceId, facet );
	}

	void notifyChanges()
	{
		checkRegistered();
		_universe->notifyChanges();
	}

	void addGraphObserver( ca::IGraphObserver* observer )
	{
		checkRegistered();
		_universe->spaceAddGraphObserver( _spaceId, observer );
	}

	void removeGraphObserver( ca::IGraphObserver* observer )
	{
		checkRegistered();
		_universe->spaceRemoveGraphObserver( _spaceId, observer );
	}

	void addObjectObserver( co::IObject* object, ca::IObjectObserver* observer )
	{
		checkRegistered();
		_universe->addObjectObserver( object, observer );
	}

	void removeObjectObserver( co::IObject* object, ca::IObjectObserver* observer )
	{
		
		checkRegistered();
		_universe->removeObjectObserver( object, observer );
	}

	void addServiceObserver( co::IService* service, ca::IServiceObserver* observer )
	{
		checkRegistered();
		_universe->addServiceObserver( service, observer );
	}

	void removeServiceObserver( co::IService* service, ca::IServiceObserver* observer )
	{
		checkRegistered();
		_universe->removeServiceObserver( service, observer );
	}

protected:
	ca::IUniverse* getUniverseService()
	{
		return _universe.get();
	}

	void setUniverseService( ca::IUniverse* universe )
	{
		if( _universe.isValid() )
			throw co::IllegalStateException( "once set, the universe of a ca.Space cannot be changed" );

		CHECK_NULL_ARG( universe );
		checkComponent( universe, "ca.Universe" );

		_universe = static_cast<Universe*>( universe );
		_spaceId = _universe->spaceRegister( this );
	}

private:
	inline void checkRegistered()
	{
		if( !_universe.isValid() )
			throw co::IllegalStateException( "the ca.Space requires a universe for this operation" );
	}

private:
	co::int16 _spaceId;
	co::RefPtr<Universe> _universe;
};

CORAL_EXPORT_COMPONENT( Space, Space )

} // namespace ca
