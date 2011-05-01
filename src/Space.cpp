/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Space_Base.h"
#include <ca/IUniverse.h>
#include <co/Coral.h>
#include <co/RefPtr.h>
#include <co/IComponent.h>
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
		// it's highly recommended that observers unregister themselves
		assert( _spaceObservers.empty() );

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

	void addChange( co::IService* facet )
	{
		checkRegistered();
		return _universe->addChange( _spaceId, facet );
	}

	void notifyChanges()
	{
		checkRegistered();
		_universe->notifyChanges( _spaceId );
	}

	void onSpaceChanged( ca::ISpaceChanges* changes )
	{
		size_t numObservers = _spaceObservers.size();
		for( size_t i = 0; i < numObservers; ++i )
			_spaceObservers[i]->onSpaceChanged( changes );
	}

	void addSpaceObserver( ca::ISpaceObserver* observer )
	{
		assert( observer != this );

		if( !observer )
			throw co::IllegalArgumentException( "illegal null observer" );

		_spaceObservers.push_back( observer );
	}

	void removeSpaceObserver( ca::ISpaceObserver* observer )
	{
		if( !observer )
			throw co::IllegalArgumentException( "illegal null observer" );

		co::int32 numObservers = static_cast<co::int32>( _spaceObservers.size() );
		co::int32 numRemaining = numObservers;
		for( co::int32 i = numObservers - 1; i >= 0; --i )
			if( _spaceObservers[i] == observer )
				std::swap( _spaceObservers[i], _spaceObservers[--numRemaining] );

		co::int32 numRemoved = ( numObservers - numRemaining );
		if( numRemoved == 0 )
			throw co::IllegalArgumentException( "observer is not in the list of space observers" );
		else if( numRemoved > 1 )
			co::debug( co::Dbg_Warning, "ca.Space: observer (%s)%p was removed %i times.",
				observer->getProvider()->getComponent()->getFullName().c_str(), observer, numRemoved );

		_spaceObservers.resize( numRemaining );
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

		if( !universe )
			throw co::IllegalArgumentException( "illegal null universe" );

		_spaceId = universe->registerSpace( this );
		_universe = universe;
	}

private:
	inline void checkRegistered()
	{
		if( !_universe.isValid() )
			throw co::IllegalStateException( "the ca.Space requires a universe for this operation" );
	}

private:
	co::int16 _spaceId;
	co::RefPtr<ca::IUniverse> _universe;
	std::vector<ca::ISpaceObserver*> _spaceObservers;
};

CORAL_EXPORT_COMPONENT( Space, Space )

} // namespace ca
