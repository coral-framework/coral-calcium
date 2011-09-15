/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Space_Base.h"
#include <ca/IUniverse.h>
#include <ca/ISpaceChanges.h>
#include <ca/IObjectChanges.h>
#include <ca/IServiceChanges.h>
#include <ca/ISpaceObserver.h>
#include <ca/IObjectObserver.h>
#include <ca/IServiceObserver.h>
#include <co/Log.h>
#include <co/Coral.h>
#include <co/RefPtr.h>
#include <co/IComponent.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>
#include <algorithm>
#include <map>

#define CHECK_NULL_ARG( name ) \
	if( !name ) throw co::IllegalArgumentException( "illegal null " #name );

namespace ca {

template<typename Observer>
inline void checkObserverRemoved( int numRemoved, Observer* observer )
{
	if( numRemoved == 0 )
		throw co::IllegalArgumentException( "no such observer in the list of observers" );

	if( numRemoved > 1 )
	{
		const char* typeName = "null";
		if( observer )
			typeName = observer->getProvider()->getComponent()->getFullName().c_str();

		CORAL_LOG(WARNING) << "ca.Space: observer (" << typeName << ')' <<
			observer << " was removed " << numRemoved << " times.";
	}
}

template<typename Container, typename Observer>
inline void removeObserver( Container& c, Observer* observer )
{
	typename Container::iterator end = c.end();
	typename Container::iterator newEnd = std::remove( c.begin(), end, observer );
	int numRemoved = static_cast<int>( std::distance( newEnd, end ) );
	c.erase( newEnd, end );
	checkObserverRemoved( numRemoved, observer );
}

template<typename Container, typename Iterator, typename Observer>
inline void removeObserver( Container& c, Iterator begin, Iterator end, Observer* observer )
{
	int numRemoved = 0;
	while( begin != end )
	{
		if( begin->second == observer )
		{
			c.erase( begin++ );
			++numRemoved;
		}
		else
		{
			++begin;
		}
	}
	checkObserverRemoved( numRemoved, observer );
}

class Space : public Space_Base
{
public:
	Space()
	{
		_spaceId = -1;
	}

	virtual ~Space()
	{
		// it is highly recommended that all observers unregister themselves
		assert( _spaceObservers.empty() );

		if( _universe.isValid() )
			_universe->unregisterSpace( _spaceId );
	}
	
	co::IObject* getRootObject()
	{
		checkRegistered();
		return _universe->getRootObject( _spaceId );
	}

	void setRootObject( co::IObject* root )
	{
		checkRegistered();
		_universe->setRootObject( _spaceId, root );
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
		// notify all space observers
		size_t numSpaceObservers = _spaceObservers.size();
		for( size_t i = 0; i < numSpaceObservers; ++i )
			_spaceObservers[i]->onSpaceChanged( changes );

		// notify observers registered for specific objects/services in the change set
		co::Range<ca::IObjectChanges* const> changedObjects = changes->getChangedObjects();
		for( ; changedObjects; changedObjects.popFirst() )
		{
			ca::IObjectChanges* objectChanges = changedObjects.getFirst();
			co::IObject* object = objectChanges->getObject();
			ObjectObserverMap::iterator it = _objectObserverMap.find( object );
			if( it == _objectObserverMap.end() )
				continue;

			// notify object observers
			ObjectObserverList& ool = it->second.objectObservers;
			size_t numObjectObservers = ool.size();
			for( size_t i = 0; i < numObjectObservers; ++i )
				ool[i]->onObjectChanged( objectChanges );

			// notify observers registered for specific services
			co::Range<IServiceChanges* const> changedServices = objectChanges->getChangedServices();
			for( ; changedServices; changedServices.popFirst() )
			{
				ca::IServiceChanges* serviceChanges = changedServices.getFirst();
				co::IService* service = serviceChanges->getService();
				ServiceObserverMapRange range = it->second.serviceObserverMap.equal_range( service );
				for( ; range.first != range.second; ++range.first )
					range.first->second->onServiceChanged( serviceChanges );
			}
		}
	}

	void addSpaceObserver( ca::ISpaceObserver* observer )
	{
		assert( observer != this );
		CHECK_NULL_ARG( observer );
		_spaceObservers.push_back( observer );
	}

	void removeSpaceObserver( ca::ISpaceObserver* observer )
	{
		removeObserver( _spaceObservers, observer );
	}

	void addObjectObserver( co::IObject* object, ca::IObjectObserver* observer )
	{
		CHECK_NULL_ARG( object );
		CHECK_NULL_ARG( observer );
		_objectObserverMap[object].objectObservers.push_back( observer );
	}

	void removeObjectObserver( co::IObject* object, ca::IObjectObserver* observer )
	{
		ObjectObserverMap::iterator it = _objectObserverMap.find( object );
		if( it == _objectObserverMap.end() )
			throw co::IllegalArgumentException( "object is not being observed" );

		ObjectObserverList& ool = it->second.objectObservers;
		removeObserver( ool, observer );

		if( ool.empty() && it->second.serviceObserverMap.empty() )
			_objectObserverMap.erase( it );
	}

	void addServiceObserver( co::IService* service, ca::IServiceObserver* observer )
	{
		CHECK_NULL_ARG( service );
		CHECK_NULL_ARG( observer );

		co::IObject* provider = service->getProvider();
		if( service == provider )
			throw co::IllegalArgumentException( "illegal service type; for services "
				"of type 'co.IObject' please call addObjectObserver() instead" );

		_objectObserverMap[provider].serviceObserverMap.insert(
			ServiceObserverMap::value_type( service, observer ) );
	}

	void removeServiceObserver( co::IService* service, ca::IServiceObserver* observer )
	{
		CHECK_NULL_ARG( service );
		ObjectObserverMap::iterator it = _objectObserverMap.find( service->getProvider() );
		if( it == _objectObserverMap.end() )
			throw co::IllegalArgumentException( "service is not being observed" );

		ServiceObserverMap& som = it->second.serviceObserverMap;
		ServiceObserverMapRange range = som.equal_range( service );
		removeObserver( som, range.first, range.second, observer );

		if( som.empty() && it->second.objectObservers.empty() )
			_objectObserverMap.erase( it );
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

	typedef std::vector<ca::ISpaceObserver*> SpaceObserverList;
	SpaceObserverList _spaceObservers;

	typedef std::vector<ca::IObjectObserver*> ObjectObserverList;
	typedef std::multimap<co::IService*, ca::IServiceObserver*> ServiceObserverMap;
	typedef std::pair<ServiceObserverMap::iterator, ServiceObserverMap::iterator> ServiceObserverMapRange;
	struct ObjectObservers
	{
		ObjectObserverList objectObservers;
		ServiceObserverMap serviceObserverMap;
	};

	typedef std::map<co::IObject*, ObjectObservers> ObjectObserverMap;
	ObjectObserverMap _objectObserverMap;
};

CORAL_EXPORT_COMPONENT( Space, Space )

} // namespace ca
