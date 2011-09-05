/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ERMSpace.h"
#include <co/IllegalStateException.h>
#include <ca/UnexpectedException.h>

class SpaceTests : public ERMSpace
{};

class SpaceTestsFaulty : public ERMSpace
{
	const char* getModelName() { return "faulty"; }
};

TEST_F( SpaceTests, initialization )
{
	// the space is empty, so beginChange() should always fail
	EXPECT_THROW( _space->addChange( _model.get() ), ca::NoSuchObjectException );

	createSimpleERM();

	// none of the created components are in the space yet
	EXPECT_THROW( _space->addChange( _erm.get() ), ca::NoSuchObjectException );
	EXPECT_THROW( _space->addChange( _entityA.get() ), ca::NoSuchObjectException );
	EXPECT_THROW( _space->addChange( _entityB.get() ), ca::NoSuchObjectException );
	EXPECT_THROW( _space->addChange( _relAB.get() ), ca::NoSuchObjectException );

	// set the graph's root object (the erm.Model)
	_space->setRootObject( _erm->getProvider() );

	// once set, the root object cannot be changed
	EXPECT_THROW( _space->setRootObject( NULL ), co::IllegalStateException );	
	EXPECT_THROW( _space->setRootObject( _entityA->getProvider() ), co::IllegalStateException );

	// now the space should contain the whole graph
	EXPECT_NO_THROW( _space->addChange( _erm.get() ) );
	EXPECT_NO_THROW( _space->addChange( _entityA.get() ) );
	EXPECT_NO_THROW( _space->addChange( _entityB.get() ) );
	EXPECT_NO_THROW( _space->addChange( _relAB.get() ) );

	// make sure notifyChanges() works
	EXPECT_FALSE( _changes.isValid() );
	_space->notifyChanges();
	ASSERT_TRUE( _changes.isValid() );

	// the initial notification should contain only 'addedObjects'
	EXPECT_EQ( _changes->getSpace(), _space.get() );
	EXPECT_FALSE( _changes->getAddedObjects().isEmpty() );
	EXPECT_TRUE( _changes->getRemovedObjects().isEmpty() );
	EXPECT_TRUE( _changes->getChangedObjects().isEmpty() );

	// test findAddedObject()
	EXPECT_TRUE( _changes->findAddedObject( NULL ) < 0 );
	EXPECT_TRUE( _changes->findAddedObject( _entityC->getProvider() ) < 0 );
	EXPECT_TRUE( _changes->findAddedObject( _relBC->getProvider() ) < 0 );
	EXPECT_TRUE( _changes->findAddedObject( _relCA->getProvider() ) < 0 );
	EXPECT_TRUE( _changes->findAddedObject( _entityA->getProvider() ) >= 0 );

	// test findRemovedObject/findChangedObject() with garbage
	EXPECT_TRUE( _changes->findRemovedObject( NULL ) < 0 );
	EXPECT_TRUE( _changes->findRemovedObject( _entityC->getProvider() ) < 0 );
	EXPECT_TRUE( _changes->findChangedObject( NULL ) < 0 );
	EXPECT_TRUE( _changes->findChangedObject( _entityC->getProvider() ) < 0 );
}

TEST_F( SpaceTests, noChange )
{
	startWithSimpleERM();

	// call addChange() on non-modified objects (should issue warnings)
	EXPECT_NO_THROW( _space->addChange( _erm.get() ) );
	EXPECT_NO_THROW( _space->addChange( _entityA.get() ) );
	EXPECT_NO_THROW( _space->addChange( _relAB.get() ) );

	// notifyChanges() should not send any notification
	_space->notifyChanges();
	EXPECT_FALSE( _changes.isValid() );
}

TEST_F( SpaceTests, simpleAdditions )
{
	startWithSimpleERM();

	// add some objects to the graph
	extendSimpleERM();

	// changes are not detected until we call addChange() on an existing node
	_space->notifyChanges();
	ASSERT_FALSE( _changes.isValid() );

	// among the existing graph nodes, only '_erm' was changed
	_space->addChange( _erm.get() );

	// this next notification should produce a 'full diff'
	ASSERT_FALSE( _changes.isValid() );
	_space->notifyChanges();
	ASSERT_TRUE( _changes.isValid() );

	// we expect 3 new objects
	EXPECT_EQ( 3, _changes->getAddedObjects().getSize() );
	EXPECT_TRUE( _changes->findAddedObject( _entityC->getProvider() ) >= 0 );
	EXPECT_TRUE( _changes->findAddedObject( _relBC->getProvider() ) >= 0 );
	EXPECT_TRUE( _changes->findAddedObject( _relCA->getProvider() ) >= 0 );

	// we expect no removed object
	EXPECT_TRUE( _changes->getRemovedObjects().isEmpty() );

	// we also expect 2 changed fields in one service (_erm's lists of entities/rels)
	co::Range<ca::IObjectChanges* const> changedObjects = _changes->getChangedObjects();
	ASSERT_EQ( 1, changedObjects.getSize() );
	EXPECT_EQ( changedObjects[0]->getObject(), _erm->getProvider() );

	co::Range<ca::IServiceChanges* const> changedServices = changedObjects[0]->getChangedServices();
	ASSERT_EQ( 1, changedServices.getSize() );

	ca::IServiceChanges* changedService = changedServices[0];
	EXPECT_EQ( _erm.get(), changedService->getService() );
	EXPECT_TRUE( changedService->getChangedRefFields().isEmpty() );
	EXPECT_TRUE( changedService->getChangedValueFields().isEmpty() );

	co::Range<ca::ChangedRefVecField const> changedRefVecs = changedService->getChangedRefVecFields();
	EXPECT_EQ( 2, changedRefVecs.getSize() );
	EXPECT_EQ( "entities", changedRefVecs[0].field->getName() );
	EXPECT_EQ( "relationships", changedRefVecs[1].field->getName() );
}

TEST_F( SpaceTests, changedReceptacles )
{
	startWithSimpleERM();

	// invert the ends of the relationship by setting its receptacles
	co::IObject* relAB = _relAB->getProvider();
	relAB->setService( "entityA", _entityB.get() );
	relAB->setService( "entityB", _entityA.get() );
	_space->addChange( relAB );
	_space->notifyChanges();
	ASSERT_TRUE( _changes.isValid() );

	// we expect only 1 changed object with 2 changed connections
	EXPECT_TRUE( _changes->getAddedObjects().isEmpty() );
	EXPECT_TRUE( _changes->getRemovedObjects().isEmpty() );

	co::Range<ca::IObjectChanges* const> changedObjects = _changes->getChangedObjects();
	ASSERT_EQ( 1, changedObjects.getSize() );
	EXPECT_EQ( changedObjects[0]->getObject(), relAB );

	co::Range<ca::ChangedConnection const> changedConnections = changedObjects[0]->getChangedConnections();
	EXPECT_EQ( 2, changedConnections.getSize() );
	EXPECT_EQ( "entityA", changedConnections[0].receptacle->getName() );
	EXPECT_EQ( _entityA.get(), changedConnections[0].previous.get() );
	EXPECT_EQ( _entityB.get(), changedConnections[0].current.get() );
	EXPECT_EQ( "entityB", changedConnections[1].receptacle->getName() );
	EXPECT_EQ( _entityB.get(), changedConnections[1].previous.get() );
	EXPECT_EQ( _entityA.get(), changedConnections[1].current.get() );
}

TEST_F( SpaceTests, changedRefFields )
{
	startWithSimpleERM();

	// make entityC (which is off the graph) a parent of entityA
	_entityA->setParent( _entityC.get() );
	_space->addChange( _entityA.get() );
	_space->notifyChanges();
	ASSERT_TRUE( _changes.isValid() );

	// we expect 1 added and 1 changed object, with 1 changed Ref field
	EXPECT_EQ( 1, _changes->getAddedObjects().getSize() );
	EXPECT_TRUE( _changes->findAddedObject( _entityC->getProvider() ) >= 0 );
	EXPECT_TRUE( _changes->getRemovedObjects().isEmpty() );

	co::Range<ca::IObjectChanges* const> changedObjects = _changes->getChangedObjects();
	ASSERT_EQ( 1, changedObjects.getSize() );
	EXPECT_EQ( _entityA->getProvider(), changedObjects[0]->getObject() );

	co::Range<ca::IServiceChanges* const> changedServices = changedObjects[0]->getChangedServices();
	ASSERT_EQ( 1, changedServices.getSize() );

	ca::IServiceChanges* changedService = changedServices[0];
	EXPECT_EQ( _entityA.get(), changedService->getService() );

	co::Range<ca::ChangedRefField const> changedRefFields = changedService->getChangedRefFields();
	ASSERT_EQ( 1, changedRefFields.getSize() );
	EXPECT_EQ( "parent", changedRefFields[0].field->getName() );
	EXPECT_EQ( NULL, changedRefFields[0].previous.get() );
	EXPECT_EQ( _entityC.get(), changedRefFields[0].current.get() );
	EXPECT_TRUE( changedService->getChangedRefVecFields().isEmpty() );
	EXPECT_TRUE( changedService->getChangedValueFields().isEmpty() );

	// now if we reset the same field to NULL, we shall get the reverse effect
	_entityA->setParent( NULL );
	_space->addChange( _entityA.get() );
	_space->notifyChanges();

	// we expect 1 removed and 1 changed object, with 1 changed Ref field
	EXPECT_TRUE( _changes->getAddedObjects().isEmpty() );
	EXPECT_EQ( 1, _changes->getRemovedObjects().getSize() );
	EXPECT_TRUE( _changes->findRemovedObject( _entityC->getProvider() ) >= 0 );

	changedObjects = _changes->getChangedObjects();
	ASSERT_EQ( 1, changedObjects.getSize() );
	EXPECT_EQ( _entityA->getProvider(), changedObjects[0]->getObject() );

	changedServices = changedObjects[0]->getChangedServices();
	ASSERT_EQ( 1, changedServices.getSize() );

	changedService = changedServices[0];
	EXPECT_EQ( _entityA.get(), changedService->getService() );

	changedRefFields = changedService->getChangedRefFields();
	ASSERT_EQ( 1, changedRefFields.getSize() );
	EXPECT_EQ( "parent", changedRefFields[0].field->getName() );
	EXPECT_EQ( _entityC.get(), changedRefFields[0].previous.get() );
	EXPECT_EQ( NULL, changedRefFields[0].current.get() );
	EXPECT_TRUE( changedService->getChangedRefVecFields().isEmpty() );
	EXPECT_TRUE( changedService->getChangedValueFields().isEmpty() );
}

TEST_F( SpaceTests, changedRefVecFields )
{
	startWithSimpleERM();

	/*
		The ERM entity list is currently { entityA, entityB }
		We'll turn it into { entityA, NULL, entityA, entityC }
	 */
	co::Range<erm::IEntity* const> entities = _erm->getEntities();
	entities.popLast();
	_erm->setEntities( entities );
	_erm->addEntity( NULL );
	_erm->addEntity( entities.getFirst() );
	_erm->addEntity( _entityC.get() );

	/*
		The ERM relationship list is currently { relAB }
		We'll turn it into { NULL, relAB, relBC, relAB }
	 */
	erm::IRelationship* rels[] = { NULL, _relAB.get(), _relBC.get(), _relAB.get() };
	_erm->setRelationships( co::Range<erm::IRelationship* const>( rels, CORAL_ARRAY_LENGTH( rels ) ) );

	// check all changes at once
	_space->addChange( _erm.get() );
	_space->notifyChanges();
	ASSERT_TRUE( _changes.isValid() );

	// we expect 2 added objects (entityC and relBC)
	EXPECT_EQ( 2, _changes->getAddedObjects().getSize() );
	EXPECT_TRUE( _changes->findAddedObject( _entityC->getProvider() ) >= 0 );
	EXPECT_TRUE( _changes->findAddedObject( _relBC->getProvider() ) >= 0 );
	EXPECT_FALSE( _changes->findAddedObject( _relCA->getProvider() ) >= 0 );

	// we expect no removed object (since relAB still has a ref to entityB)
	EXPECT_TRUE( _changes->getRemovedObjects().isEmpty() );

	// we expect 1 changed object/service (erm), with 2 changed RefVec fields
	co::Range<ca::IObjectChanges* const> changedObjects = _changes->getChangedObjects();
	ASSERT_EQ( 1, changedObjects.getSize() );
	EXPECT_EQ( _erm->getProvider(), changedObjects[0]->getObject() );

	co::Range<ca::IServiceChanges* const> changedServices = changedObjects[0]->getChangedServices();
	ASSERT_EQ( 1, changedServices.getSize() );

	ca::IServiceChanges* changedService = changedServices[0];
	EXPECT_EQ( _erm.get(), changedService->getService() );

	EXPECT_TRUE( changedService->getChangedRefFields().isEmpty() );
	EXPECT_TRUE( changedService->getChangedValueFields().isEmpty() );

	co::Range<ca::ChangedRefVecField const> changedRefVecFields = changedService->getChangedRefVecFields();
	ASSERT_EQ( 2, changedRefVecFields.getSize() );

	EXPECT_EQ( "entities", changedRefVecFields[0].field->getName() );
	ASSERT_EQ( 2, changedRefVecFields[0].previous.size() );
	EXPECT_EQ( _entityA.get(), changedRefVecFields[0].previous[0].get() );
	EXPECT_EQ( _entityB.get(), changedRefVecFields[0].previous[1].get() );
	ASSERT_EQ( 4, changedRefVecFields[0].current.size() );
	EXPECT_EQ( _entityA.get(), changedRefVecFields[0].current[0].get() );
	EXPECT_EQ( NULL, changedRefVecFields[0].current[1].get() );
	EXPECT_EQ( _entityA.get(), changedRefVecFields[0].current[2].get() );
	EXPECT_EQ( _entityC.get(), changedRefVecFields[0].current[3].get() );

	EXPECT_EQ( "relationships", changedRefVecFields[1].field->getName() );
	ASSERT_EQ( 1, changedRefVecFields[1].previous.size() );
	EXPECT_EQ( _relAB.get(), changedRefVecFields[1].previous[0].get() );
	ASSERT_EQ( 4, changedRefVecFields[1].current.size() );
	EXPECT_EQ( NULL, changedRefVecFields[1].current[0].get() );
	EXPECT_EQ( _relAB.get(), changedRefVecFields[1].current[1].get() );
	EXPECT_EQ( _relBC.get(), changedRefVecFields[1].current[2].get() );
	EXPECT_EQ( _relAB.get(), changedRefVecFields[1].current[3].get() );

	// remove relAB's ref to entityB; entityB should be removed from the space
	_relAB->setEntityB( NULL );
	_space->addChange( _relAB.get() );

	// notice: changing field 'entityB' also changes the receptacle 'entityB'
	_space->addChange( _relAB->getProvider() ); // detect connection changes

	_space->notifyChanges();
	ASSERT_TRUE( _changes.isValid() );

	// we expect one removed object (entityB) and 1 changed object/service
	EXPECT_TRUE( _changes->getAddedObjects().isEmpty() );

	EXPECT_EQ( 1, _changes->getRemovedObjects().getSize() );
	EXPECT_TRUE( _changes->findRemovedObject( _entityB->getProvider() ) >= 0 );

	// the changed object (relAB) has 1 changed field and 1 changed connection
	changedObjects = _changes->getChangedObjects();
	ASSERT_EQ( 1, changedObjects.getSize() );
	EXPECT_EQ( _relAB->getProvider(), changedObjects[0]->getObject() );
	EXPECT_EQ( 1, changedObjects[0]->getChangedConnections().getSize() );
	EXPECT_EQ( "entityB", changedObjects[0]->getChangedConnections()[0].receptacle->getName() );

	changedServices = changedObjects[0]->getChangedServices();
	ASSERT_EQ( 1, changedServices.getSize() );
	EXPECT_EQ( _relAB.get(), changedServices[0]->getService() );
	ASSERT_EQ( 1, changedServices[0]->getChangedRefFields().getSize() );
	EXPECT_EQ( "entityB", changedServices[0]->getChangedRefFields().getFirst().field->getName() );
	EXPECT_TRUE( changedServices[0]->getChangedRefVecFields().isEmpty() );
	EXPECT_TRUE( changedServices[0]->getChangedValueFields().isEmpty() );
}

TEST_F( SpaceTests, changedValueFields )
{
	startWithExtendedERM();

	// change entityA's name
	_entityA->setName( "New Name" );
	_space->addChange( _entityA.get() );

	// change relAB's "relation" string and multiplicity value
	_relBC->setRelation( "New Relation" );
	erm::Multiplicity multA = _relBC->getMultiplicityA();
	multA.min = 3;
	multA.max = 9;
	_relBC->setMultiplicityA( multA );
	_space->addChange( _relBC.get() );

	// check all changes at once
	_space->notifyChanges();
	ASSERT_TRUE( _changes.isValid() );

	// we expect 0 added, 0 removed and 2 changed objects
	EXPECT_TRUE( _changes->getAddedObjects().isEmpty() );
	EXPECT_TRUE( _changes->getRemovedObjects().isEmpty() );

	co::Range<ca::IObjectChanges* const> changedObjects = _changes->getChangedObjects();
	ASSERT_EQ( 2, changedObjects.getSize() );

	co::int32 indexOfEntityA = _changes->findChangedObject( _entityA->getProvider() );
	co::int32 indexOfRelBC = _changes->findChangedObject( _relBC->getProvider() );
	ASSERT_TRUE( indexOfEntityA >= 0 );
	ASSERT_TRUE( indexOfRelBC >= 0 );

	ca::IObjectChanges* entityAChanges = changedObjects[indexOfEntityA];
	ca::IObjectChanges* relBCChanges = changedObjects[indexOfRelBC];

	EXPECT_EQ( _entityA->getProvider(), entityAChanges->getObject() );
	ASSERT_EQ( 1, entityAChanges->getChangedServices().getSize() );
	EXPECT_EQ( 0, entityAChanges->getChangedConnections().getSize() );
	EXPECT_EQ( _relBC->getProvider(), relBCChanges->getObject() );
	ASSERT_EQ( 1, relBCChanges->getChangedServices().getSize() );
	EXPECT_EQ( 0, relBCChanges->getChangedConnections().getSize() );

	// check changes to entityA	
	ca::IServiceChanges* changedService = entityAChanges->getChangedServices().getFirst();
	EXPECT_EQ( _entityA.get(), changedService->getService() );	
	EXPECT_TRUE( changedService->getChangedRefFields().isEmpty() );
	EXPECT_TRUE( changedService->getChangedRefVecFields().isEmpty() );
	
	co::Range<ca::ChangedValueField const> changedValueFields = changedService->getChangedValueFields();
	ASSERT_EQ( 1, changedValueFields.getSize() );
	EXPECT_EQ( "name", changedValueFields[0].field->getName() );
	EXPECT_EQ( "Entity A", changedValueFields[0].previous.get<const std::string&>() );
	EXPECT_EQ( "New Name", changedValueFields[0].current.get<const std::string&>() );

	// check changes to relAB	
	changedService = relBCChanges->getChangedServices().getFirst();
	EXPECT_EQ( _relBC.get(), changedService->getService() );	
	EXPECT_TRUE( changedService->getChangedRefFields().isEmpty() );
	EXPECT_TRUE( changedService->getChangedRefVecFields().isEmpty() );

	changedValueFields = changedService->getChangedValueFields();
	ASSERT_EQ( 2, changedValueFields.getSize() );
	EXPECT_EQ( "multiplicityA", changedValueFields[0].field->getName() );
	EXPECT_EQ( 0, changedValueFields[0].previous.get<const erm::Multiplicity&>().min );
	EXPECT_EQ( 0, changedValueFields[0].previous.get<const erm::Multiplicity&>().max );
	EXPECT_EQ( 3, changedValueFields[0].current.get<const erm::Multiplicity&>().min );
	EXPECT_EQ( 9, changedValueFields[0].current.get<const erm::Multiplicity&>().max );
	EXPECT_EQ( "relation", changedValueFields[1].field->getName() );
	EXPECT_EQ( "relation B-C", changedValueFields[1].previous.get<const std::string&>() );
	EXPECT_EQ( "New Relation", changedValueFields[1].current.get<const std::string&>() );
}

TEST_F( SpaceTestsFaulty, unexpectedExceptions )
{
	createSimpleERM();
	ASSERT_EXCEPTION( _space->setRootObject( _erm->getProvider() ), "raised by field 'throwsOnGetAndSet'" );
	ASSERT_EXCEPTION( _space->setRootObject( _erm->getProvider() ), "raised by field 'throwsOnGetAndSet'" );
}
