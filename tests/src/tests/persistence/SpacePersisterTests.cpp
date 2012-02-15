/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */
#include "../ERMSpace.h"

#include "persistence/sqlite/sqlite3.h"

#include <gtest/gtest.h>

#include <co/Coral.h>
#include <co/IObject.h>
#include <ca/ISpacePersister.h>
#include <co/RefPtr.h>
#include <co/Coral.h>
#include <co/IObject.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>
#include <erm/IModel.h>
#include <erm/IRelationship.h>
#include <erm/IEntity.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/INamed.h>
#include <ca/IUniverse.h>
#include <ca/ModelException.h>
#include <ca/NoSuchObjectException.h>
#include <ca/IOException.h>
#include <ca/ISpaceStore.h>
#include <cstdio>


class SpacePersisterTests : public ERMSpace 
{
public:

	void SetUp()
	{
		ERMSpace::SetUp();
		startWithExtendedERM();
		universeObj = co::newInstance( "ca.Universe" );
		universeObj->setService("model", _model.get());
	}

	co::RefPtr<ca::ISpacePersister> createPersister( const std::string& fileName )
	{
		ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

		co::IObject* persisterObj = co::newInstance( "ca.SpacePersister" );
		co::RefPtr<ca::ISpacePersister> persister = persisterObj->getService<ca::ISpacePersister>();

		co::RefPtr<co::IObject> spaceFileObj = co::newInstance( "ca.SQLiteSpaceStore" );
		spaceFileObj->getService<ca::INamed>()->setName( fileName );

		persisterObj->setService( "store", spaceFileObj->getService<ca::ISpaceStore>() );
		persisterObj->setService( "universe", universe );

		return persister;
	}
private:
	co::RefPtr<co::IObject> universeObj;

};

void applyValueFieldChange( ca::ISpace* spaceERM )
{
	co::RefPtr<co::IObject> objRest = spaceERM->getRootObject();

	co::RefPtr<erm::IModel> serviceModel = objRest->getService<erm::IModel>();

	serviceModel->getEntities()[0]->setName( "changedName" );
	serviceModel->getRelationships()[1]->setRelation( "relationChanged" );

	spaceERM->addChange( serviceModel->getEntities()[0] );
	spaceERM->addChange( serviceModel->getRelationships()[1] );

	spaceERM->notifyChanges();
}

void applyRefVecChange( ca::ISpace* spaceERM )
{
	co::RefPtr<co::IObject> objRest = spaceERM->getRootObject();
	co::RefPtr<erm::IModel> serviceModel = objRest->getService<erm::IModel>();

	co::RefPtr<co::IObject> newEntity = co::newInstance( "erm.Entity"); 
	co::RefPtr<erm::IEntity> newIEntity = newEntity->getService<erm::IEntity>();
	newIEntity->setName( "newEntity" );
	serviceModel->addEntity( newIEntity.get() );

	spaceERM->addChange( serviceModel.get() );

	spaceERM->notifyChanges();

}

void applyAddedObjectChange( ca::ISpace* spaceERM, erm::IEntity* entity )
{
	co::IObject* objRest = spaceERM->getRootObject();
	erm::IModel* serviceModel = objRest->getService<erm::IModel>();

	co::RefPtr<co::IObject> newEntityParent = co::newInstance( "erm.Entity");
	erm::IEntity* newIEntityParent = newEntityParent->getService<erm::IEntity>();
	newIEntityParent->setName( "newEntityParent" );
	entity->setParent( newIEntityParent );

	spaceERM->addChange( entity );
	spaceERM->notifyChanges();
}

void applyChangeAndRemoveObject( ca::ISpace* spaceERM, erm::IEntity* entity )
{
	co::IObject* objRest = spaceERM->getRootObject();
	erm::IModel* serviceModel = objRest->getService<erm::IModel>();

	//entity must have a parent

	entity->getParent()->setName( "ignored change" );

	spaceERM->addChange( entity->getParent() );
	spaceERM->notifyChanges();
	
	entity->setParent( NULL );

	spaceERM->addChange( entity );
	spaceERM->notifyChanges();
}

inline erm::Multiplicity mult( co::int32 min, co::int32 max )
{
	erm::Multiplicity m;
	m.min = min; m.max = max;
	return m;
}

TEST_F( SpacePersisterTests, exceptionsTest )
{
	co::RefPtr<co::IObject> persisterObj = co::newInstance( "ca.SpacePersister" );
	ca::ISpacePersister* persister = persisterObj->getService<ca::ISpacePersister>();

	EXPECT_THROW( persister->restore(), co::IllegalStateException ); //spaceStore and universe not set
	EXPECT_THROW( persister->restoreRevision(1), co::IllegalStateException ); //spaceStore and universe not set
	
	std::string fileName = "SimpleSpaceSave.db";
	remove( fileName.c_str() );

	co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

	universeObj->setService("model", _model.get());
	
	EXPECT_THROW( persister->initialize( _erm->getProvider() ), co::IllegalStateException ); //spaceFile not set
	
	EXPECT_THROW( persister->restore(), co::IllegalStateException ); //spaceStore not set
	EXPECT_THROW( persister->restoreRevision(1), co::IllegalStateException ); //spaceStore not set

	co::RefPtr<co::IObject> spaceStoreObj = co::newInstance( "ca.SQLiteSpaceStore" );
	spaceStoreObj->getService<ca::INamed>()->setName( fileName );

	persisterObj->setService( "store", spaceStoreObj->getService<ca::ISpaceStore>() );

	remove( fileName.c_str() );
	sqlite3* hndl;
	sqlite3_open( fileName.c_str(), &hndl );
	sqlite3_close( hndl );

	EXPECT_THROW( persister->restoreRevision(1), co::IllegalArgumentException ); //empty database

}

TEST_F( SpacePersisterTests, testNewFileSetup )
{
	_relAB->setMultiplicityB( mult( 1, 2 ) );
	_relBC->setMultiplicityA( mult( 3, 4 ) );
	_relBC->setMultiplicityB( mult( 5, 6 ) );
	_relCA->setMultiplicityA( mult( 7, 8 ) );
	_relCA->setMultiplicityB( mult( 9, 0 ) );

	std::string fileName = "SimpleSpaceSave.db";
	
	remove( fileName.c_str() );

	co::RefPtr<ca::ISpacePersister> persister = createPersister( fileName );

	ASSERT_NO_THROW( persister->initialize( _erm->getProvider() ) );

	co::RefPtr<ca::ISpacePersister> persisterToRestore = createPersister( fileName );

	ASSERT_NO_THROW( persisterToRestore->restoreRevision( 1 ) );

	ca::ISpace * spaceRestored = persisterToRestore->getSpace();
	
	co::IObject* objRest = spaceRestored->getRootObject();

	erm::IModel* erm = objRest->getService<erm::IModel>();
	ASSERT_TRUE( erm != NULL );

	co::Range<erm::IEntity* const> entities = erm->getEntities();
	ASSERT_EQ( 3, entities.getSize() );

	EXPECT_EQ( "Entity A", entities[0]->getName() );
	EXPECT_EQ( "Entity B", entities[1]->getName() );
	EXPECT_EQ( "Entity C", entities[2]->getName() );

	co::Range<erm::IRelationship* const> rels = erm->getRelationships();
	ASSERT_EQ( 3, rels.getSize() );

	erm::IRelationship* rel = rels[0];
	EXPECT_EQ( 0, rel->getMultiplicityA().min );
	EXPECT_EQ( 0, rel->getMultiplicityA().max );
	EXPECT_EQ( 1, rel->getMultiplicityB().min );
	EXPECT_EQ( 2, rel->getMultiplicityB().max );
	EXPECT_EQ( "relation A-B", rel->getRelation() );
	EXPECT_EQ( entities[0], rel->getEntityA() );
	EXPECT_EQ( entities[1], rel->getEntityB() );
	
	rel = rels[1];
	EXPECT_EQ( "relation B-C", rel->getRelation() );
	EXPECT_EQ( entities[1], rel->getEntityA() );
	EXPECT_EQ( entities[2], rel->getEntityB() );
	EXPECT_EQ( 3, rel->getMultiplicityA().min );
	EXPECT_EQ( 4, rel->getMultiplicityA().max );
	EXPECT_EQ( 5, rel->getMultiplicityB().min );
	EXPECT_EQ( 6, rel->getMultiplicityB().max );

	rel = rels[2];
	EXPECT_EQ( "relation C-A", rel->getRelation() );
	EXPECT_EQ( entities[2], rel->getEntityA() );
	EXPECT_EQ( entities[0], rel->getEntityB() );
	EXPECT_EQ( 7, rel->getMultiplicityA().min );
	EXPECT_EQ( 8, rel->getMultiplicityA().max );
	EXPECT_EQ( 9, rel->getMultiplicityB().min );
	EXPECT_EQ( 0, rel->getMultiplicityB().max );
}

TEST_F( SpacePersisterTests, testSaveAccumulateChanges )
{
	_relAB->setMultiplicityB( mult( 1, 2 ) );
	_relBC->setMultiplicityA( mult( 3, 4 ) );
	_relBC->setMultiplicityB( mult( 5, 6 ) );
	_relCA->setMultiplicityA( mult( 7, 8 ) );
	_relCA->setMultiplicityB( mult( 9, 0 ) );

	std::string fileName = "SimpleSpaceSave.db";

	remove( fileName.c_str() );

	co::RefPtr<ca::ISpacePersister> persister = createPersister( fileName );
	ASSERT_NO_THROW( persister->initialize( _erm->getProvider() ) );

	ca::ISpace * spaceInitialized = persister->getSpace();

	applyValueFieldChange( spaceInitialized );
	applyRefVecChange( spaceInitialized );

	co::IObject* objRest = spaceInitialized->getRootObject();

	applyAddedObjectChange( spaceInitialized, objRest->getService<erm::IModel>()->getEntities()[3] );

	ASSERT_NO_THROW( persister->save() );

	co::RefPtr<ca::ISpacePersister> persiterRestore = createPersister( fileName );

	ASSERT_NO_THROW( persiterRestore->restore() );

	ca::ISpace * spaceRestored = persiterRestore->getSpace();

	objRest = spaceRestored->getRootObject();

	spaceRestored = persiterRestore->getSpace();

	erm::IModel* erm = objRest->getService<erm::IModel>();
	ASSERT_TRUE( erm != NULL );

	co::Range<erm::IEntity* const> entities = erm->getEntities();
	ASSERT_EQ( 4, entities.getSize() );

	co::Range<erm::IRelationship* const> rels = erm->getRelationships();
	ASSERT_EQ( 3, rels.getSize() );

	objRest = spaceRestored->getRootObject();

	erm = objRest->getService<erm::IModel>();
	ASSERT_TRUE( erm != NULL );

	EXPECT_EQ( "changedName", entities[0]->getName() );
	EXPECT_EQ( "Entity B", entities[1]->getName() );
	EXPECT_EQ( "Entity C", entities[2]->getName() );
	EXPECT_EQ( "newEntity", entities[3]->getName() );

	ASSERT_TRUE( entities[3]->getParent() != NULL );
	EXPECT_EQ( "newEntityParent", entities[3]->getParent()->getName() );

	rels = erm->getRelationships();
	ASSERT_EQ( 3, rels.getSize() );

	erm::IRelationship* rel = rels[0];
	EXPECT_EQ( 0, rel->getMultiplicityA().min );
	EXPECT_EQ( 0, rel->getMultiplicityA().max );
	EXPECT_EQ( 1, rel->getMultiplicityB().min );
	EXPECT_EQ( 2, rel->getMultiplicityB().max );
	EXPECT_EQ( "relation A-B", rel->getRelation() );
	EXPECT_EQ( entities[0], rel->getEntityA() );
	EXPECT_EQ( entities[1], rel->getEntityB() );

	rel = rels[1];
	EXPECT_EQ( "relationChanged", rel->getRelation() );
	EXPECT_EQ( entities[1], rel->getEntityA() );
	EXPECT_EQ( entities[2], rel->getEntityB() );
	EXPECT_EQ( 3, rel->getMultiplicityA().min );
	EXPECT_EQ( 4, rel->getMultiplicityA().max );
	EXPECT_EQ( 5, rel->getMultiplicityB().min );
	EXPECT_EQ( 6, rel->getMultiplicityB().max );

	rel = rels[2];
	EXPECT_EQ( "relation C-A", rel->getRelation() );
	EXPECT_EQ( entities[2], rel->getEntityA() );
	EXPECT_EQ( entities[0], rel->getEntityB() );
	EXPECT_EQ( 7, rel->getMultiplicityA().min );
	EXPECT_EQ( 8, rel->getMultiplicityA().max );
	EXPECT_EQ( 9, rel->getMultiplicityB().min );
	EXPECT_EQ( 0, rel->getMultiplicityB().max );

}


TEST_F( SpacePersisterTests, testSaveMultipleRevisions )
{
	_relAB->setMultiplicityB( mult( 1, 2 ) );
	_relBC->setMultiplicityA( mult( 3, 4 ) );
	_relBC->setMultiplicityB( mult( 5, 6 ) );
	_relCA->setMultiplicityA( mult( 7, 8 ) );
	_relCA->setMultiplicityB( mult( 9, 0 ) );

	std::string fileName = "SimpleSpaceSave.db";

	remove( fileName.c_str() );

	co::RefPtr<ca::ISpacePersister> persister = createPersister( fileName );
	ASSERT_NO_THROW( persister->initialize( _erm->getProvider() ) );

	ca::ISpace * spaceInitialized = persister->getSpace();

	co::IObject* objRest = spaceInitialized->getRootObject();
	
	erm::IModel* serviceModel = objRest->getService<erm::IModel>();

	applyValueFieldChange( spaceInitialized );

	ASSERT_NO_THROW( persister->save() );

	applyRefVecChange( spaceInitialized );

	ASSERT_NO_THROW( persister->save() );
	
	applyAddedObjectChange( spaceInitialized, serviceModel->getEntities()[3] );
	erm::IEntity* entityToDelete = objRest->getService<erm::IModel>()->getEntities()[3]->getParent();
	ASSERT_NO_THROW( persister->save() );

	applyChangeAndRemoveObject( spaceInitialized, serviceModel->getEntities()[3] );
	ASSERT_NO_THROW( persister->save() );

	co::RefPtr<ca::ISpacePersister> persiterRestore = createPersister( fileName );

	ASSERT_NO_THROW( persiterRestore->restoreRevision( 2 ) );

	ca::ISpace * spaceRestored = persiterRestore->getSpace();

	objRest = spaceRestored->getRootObject();

	erm::IModel* erm = objRest->getService<erm::IModel>();
	ASSERT_TRUE( erm != NULL );

	co::Range<erm::IEntity* const> entities = erm->getEntities();
	ASSERT_EQ( 3, entities.getSize() );

	EXPECT_EQ( "changedName", entities[0]->getName() );
	EXPECT_EQ( "Entity B", entities[1]->getName() );
	EXPECT_EQ( "Entity C", entities[2]->getName() );

	co::Range<erm::IRelationship* const> rels = erm->getRelationships();
	ASSERT_EQ( 3, rels.getSize() );

	erm::IRelationship* rel = rels[0];
	EXPECT_EQ( 0, rel->getMultiplicityA().min );
	EXPECT_EQ( 0, rel->getMultiplicityA().max );
	EXPECT_EQ( 1, rel->getMultiplicityB().min );
	EXPECT_EQ( 2, rel->getMultiplicityB().max );
	EXPECT_EQ( "relation A-B", rel->getRelation() );
	EXPECT_EQ( entities[0], rel->getEntityA() );
	EXPECT_EQ( entities[1], rel->getEntityB() );

	rel = rels[1];
	EXPECT_EQ( "relationChanged", rel->getRelation() );
	EXPECT_EQ( entities[1], rel->getEntityA() );
	EXPECT_EQ( entities[2], rel->getEntityB() );
	EXPECT_EQ( 3, rel->getMultiplicityA().min );
	EXPECT_EQ( 4, rel->getMultiplicityA().max );
	EXPECT_EQ( 5, rel->getMultiplicityB().min );
	EXPECT_EQ( 6, rel->getMultiplicityB().max );

	rel = rels[2];
	EXPECT_EQ( "relation C-A", rel->getRelation() );
	EXPECT_EQ( entities[2], rel->getEntityA() );
	EXPECT_EQ( entities[0], rel->getEntityB() );
	EXPECT_EQ( 7, rel->getMultiplicityA().min );
	EXPECT_EQ( 8, rel->getMultiplicityA().max );
	EXPECT_EQ( 9, rel->getMultiplicityB().min );
	EXPECT_EQ( 0, rel->getMultiplicityB().max );

	applyValueFieldChange( spaceRestored );

	EXPECT_THROW( persiterRestore->save(), ca::IOException ); //attempt to save when current revision is not the last

	co::RefPtr<ca::ISpacePersister> persiterRestore2 = createPersister( fileName );

	ASSERT_NO_THROW( persiterRestore2->restoreRevision( 3 ) );

	spaceRestored = persiterRestore2->getSpace();

	objRest = spaceRestored->getRootObject();

	erm = objRest->getService<erm::IModel>();
	ASSERT_TRUE( erm != NULL );

	entities = erm->getEntities();
	ASSERT_EQ( 4, entities.getSize() );

	EXPECT_EQ( "changedName", entities[0]->getName() );
	EXPECT_EQ( "Entity B", entities[1]->getName() );
	EXPECT_EQ( "Entity C", entities[2]->getName() );
	EXPECT_EQ( "newEntity", entities[3]->getName() );

	rels = erm->getRelationships();
	ASSERT_EQ( 3, rels.getSize() );

	rel = rels[0];
	EXPECT_EQ( 0, rel->getMultiplicityA().min );
	EXPECT_EQ( 0, rel->getMultiplicityA().max );
	EXPECT_EQ( 1, rel->getMultiplicityB().min );
	EXPECT_EQ( 2, rel->getMultiplicityB().max );
	EXPECT_EQ( "relation A-B", rel->getRelation() );
	EXPECT_EQ( entities[0], rel->getEntityA() );
	EXPECT_EQ( entities[1], rel->getEntityB() );

	rel = rels[1];
	EXPECT_EQ( "relationChanged", rel->getRelation() );
	EXPECT_EQ( entities[1], rel->getEntityA() );
	EXPECT_EQ( entities[2], rel->getEntityB() );
	EXPECT_EQ( 3, rel->getMultiplicityA().min );
	EXPECT_EQ( 4, rel->getMultiplicityA().max );
	EXPECT_EQ( 5, rel->getMultiplicityB().min );
	EXPECT_EQ( 6, rel->getMultiplicityB().max );

	rel = rels[2];
	EXPECT_EQ( "relation C-A", rel->getRelation() );
	EXPECT_EQ( entities[2], rel->getEntityA() );
	EXPECT_EQ( entities[0], rel->getEntityB() );
	EXPECT_EQ( 7, rel->getMultiplicityA().min );
	EXPECT_EQ( 8, rel->getMultiplicityA().max );
	EXPECT_EQ( 9, rel->getMultiplicityB().min );
	EXPECT_EQ( 0, rel->getMultiplicityB().max );

	co::RefPtr<ca::ISpacePersister> persiterRestore3 = createPersister( fileName );

	ASSERT_NO_THROW( persiterRestore3->restoreRevision( 4 ) );

	spaceRestored = persiterRestore3->getSpace();

	objRest = spaceRestored->getRootObject();

	erm = objRest->getService<erm::IModel>();
	ASSERT_TRUE( erm != NULL );

	entities = erm->getEntities();
	ASSERT_EQ( 4, entities.getSize() );

	EXPECT_EQ( "changedName", entities[0]->getName() );
	EXPECT_EQ( "Entity B", entities[1]->getName() );
	EXPECT_EQ( "Entity C", entities[2]->getName() );
	EXPECT_EQ( "newEntity", entities[3]->getName() );

	ASSERT_TRUE( entities[3]->getParent() != NULL );
	EXPECT_EQ( "newEntityParent", entities[3]->getParent()->getName() );

	rels = erm->getRelationships();
	ASSERT_EQ( 3, rels.getSize() );

	rel = rels[0];
	EXPECT_EQ( 0, rel->getMultiplicityA().min );
	EXPECT_EQ( 0, rel->getMultiplicityA().max );
	EXPECT_EQ( 1, rel->getMultiplicityB().min );
	EXPECT_EQ( 2, rel->getMultiplicityB().max );
	EXPECT_EQ( "relation A-B", rel->getRelation() );
	EXPECT_EQ( entities[0], rel->getEntityA() );
	EXPECT_EQ( entities[1], rel->getEntityB() );

	rel = rels[1];
	EXPECT_EQ( "relationChanged", rel->getRelation() );
	EXPECT_EQ( entities[1], rel->getEntityA() );
	EXPECT_EQ( entities[2], rel->getEntityB() );
	EXPECT_EQ( 3, rel->getMultiplicityA().min );
	EXPECT_EQ( 4, rel->getMultiplicityA().max );
	EXPECT_EQ( 5, rel->getMultiplicityB().min );
	EXPECT_EQ( 6, rel->getMultiplicityB().max );

	rel = rels[2];
	EXPECT_EQ( "relation C-A", rel->getRelation() );
	EXPECT_EQ( entities[2], rel->getEntityA() );
	EXPECT_EQ( entities[0], rel->getEntityB() );
	EXPECT_EQ( 7, rel->getMultiplicityA().min );
	EXPECT_EQ( 8, rel->getMultiplicityA().max );
	EXPECT_EQ( 9, rel->getMultiplicityB().min );
	EXPECT_EQ( 0, rel->getMultiplicityB().max );

	co::RefPtr<ca::ISpacePersister> persiterRestore4 = createPersister( fileName );

	ASSERT_NO_THROW( persiterRestore4->restore() );

	spaceRestored = persiterRestore4->getSpace();

	objRest = spaceRestored->getRootObject();

	erm = objRest->getService<erm::IModel>();
	ASSERT_TRUE( erm != NULL );

	entities = erm->getEntities();
	ASSERT_EQ( 4, entities.getSize() );

	EXPECT_EQ( "changedName", entities[0]->getName() );
	EXPECT_EQ( "Entity B", entities[1]->getName() );
	EXPECT_EQ( "Entity C", entities[2]->getName() );
	EXPECT_EQ( "newEntity", entities[3]->getName() );

	ASSERT_TRUE( entities[3]->getParent() == NULL );

	rels = erm->getRelationships();
	ASSERT_EQ( 3, rels.getSize() );

	rel = rels[0];
	EXPECT_EQ( 0, rel->getMultiplicityA().min );
	EXPECT_EQ( 0, rel->getMultiplicityA().max );
	EXPECT_EQ( 1, rel->getMultiplicityB().min );
	EXPECT_EQ( 2, rel->getMultiplicityB().max );
	EXPECT_EQ( "relation A-B", rel->getRelation() );
	EXPECT_EQ( entities[0], rel->getEntityA() );
	EXPECT_EQ( entities[1], rel->getEntityB() );

	rel = rels[1];
	EXPECT_EQ( "relationChanged", rel->getRelation() );
	EXPECT_EQ( entities[1], rel->getEntityA() );
	EXPECT_EQ( entities[2], rel->getEntityB() );
	EXPECT_EQ( 3, rel->getMultiplicityA().min );
	EXPECT_EQ( 4, rel->getMultiplicityA().max );
	EXPECT_EQ( 5, rel->getMultiplicityB().min );
	EXPECT_EQ( 6, rel->getMultiplicityB().max );

	rel = rels[2];
	EXPECT_EQ( "relation C-A", rel->getRelation() );
	EXPECT_EQ( entities[2], rel->getEntityA() );
	EXPECT_EQ( entities[0], rel->getEntityB() );
	EXPECT_EQ( 7, rel->getMultiplicityA().min );
	EXPECT_EQ( 8, rel->getMultiplicityA().max );
	EXPECT_EQ( 9, rel->getMultiplicityB().min );
	EXPECT_EQ( 0, rel->getMultiplicityB().max );

	erm->getEntities()[0]->setName( "another change" );
	spaceRestored->addChange( erm->getEntities()[0] );
	spaceRestored->notifyChanges();
	ASSERT_NO_THROW( persiterRestore4->save() ); //it's ok to save new revision

}
