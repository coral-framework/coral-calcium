/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include <gtest/gtest.h>

#include <co/Coral.h>
#include <co/IField.h>
#include <co/IObject.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/IUniverse.h>
#include <ca/ISpaceChanges.h>
#include <ca/ISpaceObserver.h>
#include <ca/IObjectChanges.h>
#include <ca/IServiceChanges.h>
#include <ca/ChangedRefField.h>
#include <ca/ChangedRefVecField.h>
#include <ca/ChangedValueField.h>
#include <ca/ChangedConnection.h>
#include <ca/NoSuchObjectException.h>

#include <erm/IModel.h>
#include <erm/IEntity.h>
#include <erm/IRelationship.h>
#include <erm/Multiplicity.h>

#include <algorithm>


class SpaceTests : public ::testing::Test, public ca::ISpaceObserver
{
public:
	void onSpaceChanged( ca::ISpaceChanges* changes )
	{
		_changes = changes;
	}

	co::IInterface* getInterface() { return 0; }
	co::IObject* getProvider() { return 0; }
	co::IPort* getFacet() { return 0; }
	void serviceRetain() {;}
	void serviceRelease() {;}

protected:
	virtual void SetUp()
	{
		// create an object model
		_modelObj = co::newInstance( "ca.Model" );
		_model = _modelObj->getService<ca::IModel>();
		assert( _model.isValid() );

		_model->setName( "erm" );

		// create an object universe and bind the model
		_universeObj = co::newInstance( "ca.Universe" );
		_universe = _universeObj->getService<ca::IUniverse>();
		assert( _universe.isValid() );

		_universeObj->setService( "model", _model.get() );

		// create an object space and bind it to the universe
		_spaceObj = co::newInstance( "ca.Space" );
		_space = _spaceObj->getService<ca::ISpace>();
		assert( _space.isValid() );

		_spaceObj->setService( "universe", _universe.get() );

		_space->addSpaceObserver( this );
	}

	co::IObject* createSimpleERM()
	{
		// create a simple object graph with 2 entities and 1 relationship
		_entityA = co::newInstance( "erm.Entity" )->getService<erm::IEntity>();
		_entityA->setName( "Entity A" );
		
		_entityB = co::newInstance( "erm.Entity" )->getService<erm::IEntity>();
		_entityB->setName( "Entity B" );
		
		_relAB = co::newInstance( "erm.Relationship" )->getService<erm::IRelationship>();
		_relAB->setRelation( "relation A-B" );
		_relAB->setEntityA( _entityA.get() );
		_relAB->setEntityB( _entityB.get() );
		
		_erm = co::newInstance( "erm.Model" )->getService<erm::IModel>();
		_erm->addEntity( _entityA.get() );
		_erm->addEntity( _entityB.get() );
		_erm->addRelationship( _relAB.get() );

		// 1 entity and 2 rels are created but not added to the graph yet
		_entityC = co::newInstance( "erm.Entity" )->getService<erm::IEntity>();
		_entityC->setName( "Entity C" );

		_relBC = co::newInstance( "erm.Relationship" )->getService<erm::IRelationship>();
		_relBC->setRelation( "relation B-C" );

		_relCA = co::newInstance( "erm.Relationship" )->getService<erm::IRelationship>();
		_relCA->setRelation( "relation C-A" );

		return _erm->getProvider();
	}

	void startWithSimpleERM()
	{
		_space->addRootObject( createSimpleERM() );

		// skip the first notification with the initial object additions
		_space->notifyChanges();
		_changes = NULL;
	}

	void extendSimpleERM()
	{
		// adds 1 entity and 2 relationships, closing a 'loop' in the ERM
		_relBC->getProvider()->setService( "entityA", _entityB.get() );
		_relBC->getProvider()->setService( "entityB", _entityC.get() );

		_relCA->setEntityA( _entityC.get() );
		_relCA->setEntityB( _entityA.get() );

		_erm->addEntity( _entityC.get() );
		_erm->addRelationship( _relBC.get() );
		_erm->addRelationship( _relCA.get() );
	}

	void startWithExtendedERM()
	{
		createSimpleERM();
		extendSimpleERM();
		_space->addRootObject( _erm->getProvider() );

		// skip the first notification with the initial object additions
		_space->notifyChanges();
		_changes = NULL;
	}

	virtual void TearDown()
	{
		_modelObj = NULL;
		_spaceObj = NULL;
		_universeObj = NULL;

		_model = NULL;
		_space = NULL;
		_universe = NULL;
	}

protected:
	co::RefPtr<co::IObject> _modelObj;
	co::RefPtr<co::IObject> _universeObj;
	co::RefPtr<co::IObject> _spaceObj;

	co::RefPtr<ca::IModel> _model;
	co::RefPtr<ca::IUniverse> _universe;
	co::RefPtr<ca::ISpace> _space;

	co::RefPtr<ca::ISpaceChanges> _changes;

	co::RefPtr<erm::IEntity> _entityA;
	co::RefPtr<erm::IEntity> _entityB;
	co::RefPtr<erm::IEntity> _entityC;
	co::RefPtr<erm::IRelationship> _relAB;
	co::RefPtr<erm::IRelationship> _relBC;
	co::RefPtr<erm::IRelationship> _relCA;
	co::RefPtr<erm::IModel> _erm;
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

	// add the graph's root object (the erm.Model) to the space
	_space->addRootObject( _erm->getProvider() );

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

	// test wasAdded()
	EXPECT_FALSE( _changes->wasAdded( NULL ) );
	EXPECT_FALSE( _changes->wasAdded( _entityC->getProvider() ) );
	EXPECT_FALSE( _changes->wasAdded( _relBC->getProvider() ) );
	EXPECT_FALSE( _changes->wasAdded( _relCA->getProvider() ) );
	EXPECT_TRUE( _changes->wasAdded( _entityA->getProvider() ) );

	// test wasRemoved/wasChanged() with garbage
	EXPECT_FALSE( _changes->wasRemoved( NULL ) );
	EXPECT_FALSE( _changes->wasRemoved( _entityC->getProvider() ) );
	EXPECT_FALSE( _changes->wasChanged( NULL ) );
	EXPECT_FALSE( _changes->wasChanged( _entityC->getProvider() ) );
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
	EXPECT_TRUE( _changes->wasAdded( _entityC->getProvider() ) );
	EXPECT_TRUE( _changes->wasAdded( _relBC->getProvider() ) );
	EXPECT_TRUE( _changes->wasAdded( _relCA->getProvider() ) );

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
	EXPECT_TRUE( _changes->wasAdded( _entityC->getProvider() ) );
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
	EXPECT_TRUE( _changes->wasRemoved( _entityC->getProvider() ) );

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
	EXPECT_TRUE( _changes->wasAdded( _entityC->getProvider() ) );
	EXPECT_TRUE( _changes->wasAdded( _relBC->getProvider() ) );
	EXPECT_FALSE( _changes->wasAdded( _relCA->getProvider() ) );

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
	EXPECT_TRUE( _changes->wasRemoved( _entityB->getProvider() ) );

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

	ca::IObjectChanges* changedEntityAObj = changedObjects[0];
	ca::IObjectChanges* changedRelBCObj = changedObjects[1];
	if( _entityA->getProvider() != changedEntityAObj->getObject() )
		std::swap( changedEntityAObj, changedRelBCObj );

	EXPECT_EQ( _entityA->getProvider(), changedEntityAObj->getObject() );
	ASSERT_EQ( 1, changedEntityAObj->getChangedServices().getSize() );
	EXPECT_EQ( 0, changedEntityAObj->getChangedConnections().getSize() );
	EXPECT_EQ( _relBC->getProvider(), changedRelBCObj->getObject() );
	ASSERT_EQ( 1, changedRelBCObj->getChangedServices().getSize() );
	EXPECT_EQ( 0, changedRelBCObj->getChangedConnections().getSize() );

	// check changes to entityA	
	ca::IServiceChanges* changedService = changedEntityAObj->getChangedServices().getFirst();
	EXPECT_EQ( _entityA.get(), changedService->getService() );	
	EXPECT_TRUE( changedService->getChangedRefFields().isEmpty() );
	EXPECT_TRUE( changedService->getChangedRefVecFields().isEmpty() );
	
	co::Range<ca::ChangedValueField const> changedValueFields = changedService->getChangedValueFields();
	ASSERT_EQ( 1, changedValueFields.getSize() );
	EXPECT_EQ( "name", changedValueFields[0].field->getName() );
	EXPECT_EQ( "Entity A", changedValueFields[0].previous.get<const std::string&>() );
	EXPECT_EQ( "New Name", changedValueFields[0].current.get<const std::string&>() );

	// check changes to relAB	
	changedService = changedRelBCObj->getChangedServices().getFirst();
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
