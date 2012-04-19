/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Model.h"
#include "Universe.h"
#include "LuaManager_Base.h"
#include <ca/IGraphObserver.h>
#include <co/IllegalStateException.h>

namespace ca {

class LuaManager : public LuaManager_Base, public MultiverseObserver
{
public:
	LuaManager()
	{
		Universe::setMultiverseObserver( this );
	}

	virtual ~LuaManager()
	{
		if( _universalObserver.isValid() )
		{
			size_t numUniverses = _universes.size();
			for( size_t i = 0; i < numUniverses; ++i )
				_universes[i]->removeGraphObserver( _universalObserver.get() );
		}

		Universe::setMultiverseObserver( NULL );
	}

	// ------ MultiverseObserver Methods ------ //
	
	void onUniverseCreated( Universe* universe )
	{
		if( _universalObserver.isValid() )
			universe->addGraphObserver( _universalObserver.get() );
		_universes.push_back( universe );
	}

	void onUniverseDestroyed( Universe* universe )
	{
		if( _universalObserver.isValid() )
			universe->removeGraphObserver( _universalObserver.get() );
		_universes.erase( std::remove( _universes.begin(), _universes.end(), universe ) );
	}

	// ------ ca.ILuaManager Methods ------ //

	ca::IGraphObserver* getObserver()
	{
		return _universalObserver.get();
	}

	void setObserver( ca::IGraphObserver* observer )
	{
		if( _universalObserver.isValid() )
			throw co::IllegalStateException( "the observer can only be set once" );

		_universalObserver = observer;

		size_t numUniverses = _universes.size();
		for( size_t i = 0; i < numUniverses; ++i )
			_universes[i]->addGraphObserver( observer );
	}

	// ------ lua.IInterceptor Methods ------ //

	void serviceRetained( co::IService* service )
	{
		// ignored
	}

	void serviceReleased( co::IService* service )
	{
		// ignored
	}

	void postGetService( co::IObject* object, co::IPort*, co::IService* )
	{
		// ignored
	}

	void postGetField( co::IService*, co::IField*, const co::Any& )
	{
		// ignored
	}

	void postSetService( co::IObject* object, co::IPort*, co::IService* )
	{
		tryAddChange( object );
	}

	void postSetField( co::IService* service, co::IField*, const co::Any& )
	{
		tryAddChange( service );
	}

	void postInvoke( co::IService* service, co::IMethod*, co::Range<co::Any const>, const co::Any& )
	{
		tryAddChange( service );
	}

private:
	void tryAddChange( co::IService* service )
	{
		co::IObject* object = service->getProvider();
		if( !Model::contains( object->getComponent() ) )
			return;

		size_t numUniverses = _universes.size();
		for( size_t i = 0; i < numUniverses; ++i )
			if( _universes[i]->tryAddChange( object, service ) )
				break;
	}

private:
	co::RefPtr<ca::IGraphObserver> _universalObserver;
	std::vector<Universe*> _universes;
};

CORAL_EXPORT_COMPONENT( LuaManager, LuaManager );

} // namespace ca
