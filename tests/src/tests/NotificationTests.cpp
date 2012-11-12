/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ERMSpace.h"
#include <co/MissingServiceException.h>
#include <co/IllegalArgumentException.h>
#include <ca/UnexpectedException.h>
#include <camodels/SomeInterface.h>
#include <algorithm>

class SpaceNotificationTests : public ERMSpace {};

TEST_F( SpaceNotificationTests, observersRegistration )
{
	startWithExtendedERM();

	EXPECT_THROW( _space->addServiceObserver( NULL, this ), co::IllegalArgumentException );
	EXPECT_THROW( _space->addServiceObserver( _relAB.get(), NULL ), co::IllegalArgumentException );
	EXPECT_THROW( _space->addServiceObserver( _relAB->getProvider(), this ), co::IllegalArgumentException );

	_space->addObjectObserver( _entityA->getProvider(), this );
	_space->addObjectObserver( _entityA->getProvider(), this );
	_space->addServiceObserver( _entityA.get(), this );
	_space->addServiceObserver( _entityA.get(), this );
	_space->addServiceObserver( _entityB.get(), this );

	_space->removeObjectObserver( _entityA->getProvider(), this );
	_space->removeServiceObserver( _entityA.get(), this );

	_entityA->setName( "Entity B" );
	_space->addChange( _entityA.get() );

	_entityB->setName( "Entity A" );
	_space->addChange( _entityB.get() );

	ASSERT_EQ( 0, _objectChanges.size() );
	ASSERT_EQ( 0, _serviceChanges.size() );

	_space->notifyChanges();

	ASSERT_EQ( 0, _objectChanges.size() );
	ASSERT_EQ( 1, _serviceChanges.size() );

	EXPECT_THROW( _space->removeServiceObserver( _entityB.get(), NULL ), co::IllegalArgumentException );
	EXPECT_THROW( _space->removeServiceObserver( NULL, this ), co::IllegalArgumentException );

	_space->removeServiceObserver( _entityB.get(), this );

	EXPECT_THROW( _space->removeServiceObserver( _entityB.get(), this ), co::IllegalArgumentException );
}

TEST_F( SpaceNotificationTests, objectObservers )
{
	startWithExtendedERM();

	_space->addObjectObserver( _entityA->getProvider(), this );
	_space->addObjectObserver( _entityB->getProvider(), this );
	_space->addObjectObserver( _relAB->getProvider(), this );

	_entityA->setName( "Entity B" );
	_space->addChange( _entityA.get() );

	_entityB->setName( "Entity A" );
	_space->addChange( _entityB.get() );

	_relAB->setEntityA( _entityB.get() );
	_relAB->setEntityB( _entityA.get() );
	_space->addChange( _relAB->getProvider() );

	_entityC->setName( "Entity CC" );
	_space->addChange( _entityC.get() );

	ASSERT_EQ( 0, _objectChanges.size() );
	ASSERT_EQ( 0, _serviceChanges.size() );
	
	_space->notifyChanges();

	ASSERT_EQ( 3, _objectChanges.size() );
	ASSERT_EQ( 0, _serviceChanges.size() );

	co::IObject* expectedObjects[] = {
		_entityA->getProvider(),
		_entityB->getProvider(),
		_relAB->getProvider()
	};
	const int numExpectedObjects = CORAL_ARRAY_LENGTH( expectedObjects );
	std::sort( expectedObjects, expectedObjects + numExpectedObjects );

	for( int i = 0; i < numExpectedObjects; ++i )
		EXPECT_EQ( expectedObjects[i], _objectChanges[i]->getObject() );
}

TEST_F( SpaceNotificationTests, sharedModelNotification )
{
	startWithExtendedERM();

	co::IObjectRef modelObj = co::newInstance( "ca.Model" );
	ca::IModel* model = modelObj->getService<ca::IModel>();
	model->setName( "shared" );

	co::IObjectRef universeObj = co::newInstance( "ca.Universe" );
	universeObj->setService( "model", model );
	ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

	co::IObjectRef rootObj = co::newInstance( "camodels.SomeComponent" );
	camodels::SomeInterface* someInterface = rootObj->getService<camodels::SomeInterface>();
	someInterface->setExternal( _erm.get() );

	co::IObjectRef spaceObj = co::newInstance( "ca.Space" );
	spaceObj->setService( "universe", universe );

	ca::ISpace* space = spaceObj->getService<ca::ISpace>();
	ASSERT_NO_THROW( space->initialize( rootObj.get() ) );
	ASSERT_NO_THROW( space->notifyChanges() );

	space->addServiceObserver( _entityA.get(), this );
	space->addServiceObserver( _entityB.get(), this );

	_entityA->setName( "Entity B" );
	space->addChange( _entityA.get() );

	_entityB->setName( "Entity A" );
	space->addChange( _entityB.get() );

	_entityC->setName( "Entity CC" );
	space->addChange( _entityC.get() );

	ASSERT_EQ( 0, _objectChanges.size() );
	ASSERT_EQ( 0, _serviceChanges.size() );

	space->notifyChanges();

	ASSERT_EQ( 0, _objectChanges.size() );
	ASSERT_EQ( 2, _serviceChanges.size() );

	co::IService* expectedServices[] = {
		_entityA.get(),
		_entityB.get(),
	};
	const int numExpectedServices = CORAL_ARRAY_LENGTH( expectedServices );
	std::sort( expectedServices, expectedServices + numExpectedServices );

	for( int i = 0; i < numExpectedServices; ++i )
		EXPECT_EQ( expectedServices[i], _serviceChanges[i]->getService() );
}

TEST_F( SpaceNotificationTests, serviceObservers )
{
	startWithExtendedERM();

	_space->addServiceObserver( _entityA.get(), this );
	_space->addServiceObserver( _entityB.get(), this );
	_space->addServiceObserver( _relAB.get(), this );
	_space->addObjectObserver( _relAB->getProvider(), this );

	_entityA->setName( "Entity B" );
	_space->addChange( _entityA.get() );

	_entityB->setName( "Entity A" );
	_space->addChange( _entityB.get() );

	_relAB->setEntityA( _entityB.get() );
	_relAB->setEntityB( _entityA.get() );
	_space->addChange( _relAB.get() );
	_space->addChange( _relAB->getProvider() );

	_entityC->setName( "Entity CC" );
	_space->addChange( _entityC.get() );

	ASSERT_EQ( 0, _objectChanges.size() );
	ASSERT_EQ( 0, _serviceChanges.size() );

	_space->notifyChanges();

	ASSERT_EQ( 1, _objectChanges.size() );
	ASSERT_EQ( 3, _serviceChanges.size() );

	co::IService* expectedServices[] = {
		_entityA.get(),
		_entityB.get(),
		_relAB.get()
	};
	const int numExpectedServices = CORAL_ARRAY_LENGTH( expectedServices );
	std::sort( expectedServices, expectedServices + numExpectedServices );

	for( int i = 0; i < numExpectedServices; ++i )
		EXPECT_EQ( expectedServices[i], _serviceChanges[i]->getService() );

	EXPECT_EQ( 2, _objectChanges[0]->getChangedConnections().getSize() );
}

class ObserverExceptionTests : public ERMSpace
{
public:
	const char* nextException;
	ObserverExceptionTests() : nextException( NULL ) {;}

	void onGraphChanged( ca::IGraphChanges* changes )
	{
		if( nextException )
		{
			const char* msg = nextException; nextException = NULL;
			CORAL_THROW( co::MissingServiceException, msg );
		}
	}

	void onObjectChanged( ca::IObjectChanges* changes )
	{
		if( nextException )
		{
			const char* msg = nextException; nextException = NULL;
			CORAL_THROW( co::MissingServiceException, msg );
		}
	}

	void onServiceChanged( ca::IServiceChanges* changes )
	{
		if( nextException )
		{
			const char* msg = nextException; nextException = NULL;
			CORAL_THROW( co::MissingServiceException, msg );
		}
	}
};

TEST_F( ObserverExceptionTests, spaceObserver )
{
	_space->initialize( createSimpleERM() );
	nextException = "space exception";
	ASSERT_EXCEPTION2( _space->notifyChanges(),
		"unexpected co.MissingServiceException raised by graph observer (ca.Model)",
		"space exception" );
}

TEST_F( ObserverExceptionTests, objectObserver )
{
	startWithSimpleERM();
	_space->removeGraphObserver( this );
	_space->addObjectObserver( _entityA->getProvider(), this );

	_entityA->setName( "Entity B" );
	_space->addChange( _entityA.get() );

	nextException = "object exception";

	ASSERT_EXCEPTION2( _space->notifyChanges(),
		"unexpected co.MissingServiceException raised by object observer (ca.Model)",
		"object exception" );

	_space->addGraphObserver( this );
}

TEST_F( ObserverExceptionTests, serviceObserver )
{
	startWithSimpleERM();
	_space->removeGraphObserver( this );
	_space->addServiceObserver( _entityA.get(), this );

	_entityA->setName( "Entity B" );
	_space->addChange( _entityA.get() );

	nextException = "service exception";

	ASSERT_EXCEPTION2( _space->notifyChanges(),
		"unexpected co.MissingServiceException raised by service observer (ca.Model)",
		"service exception" );

	_space->addGraphObserver( this );
}
