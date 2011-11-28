/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */
#include "ERMSpace.h"

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
#include "IResultSet.h"
#include <ca/NoSuchObjectException.h>
#include <ca/IOException.h>
#include <ca/ISpaceStore.h>
#include <cstdio>
#include <sqlite3.h>

#include "SQLiteDBConnection.h"


class SpacePersisterTest : public ERMSpace {};


inline erm::Multiplicity mult( co::int32 min, co::int32 max )
{
	erm::Multiplicity m;
	m.min = min; m.max = max;
	return m;
}

TEST_F( SpacePersisterTest, exceptionsTest )
{
	co::RefPtr<co::IObject> persisterObj = co::newInstance( "ca.SpacePersister" );
	ca::ISpacePersister* persister = persisterObj->getService<ca::ISpacePersister>();

	EXPECT_THROW( persister->restore(), co::IllegalStateException ); //spaceStore and universe not set
	EXPECT_THROW( persister->restoreRevision(1), co::IllegalStateException ); //spaceStore and universe not set
	
	std::string fileName = "SimpleSpaceSave.db";
	remove( fileName.c_str() );

	co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

	startWithExtendedERM();

	universeObj->setService("model", _model.get());
	
	EXPECT_THROW( persister->initialize( _erm->getProvider() ), co::IllegalStateException ); //spaceFile not set
	
	EXPECT_THROW( persister->restore(), co::IllegalStateException ); //spaceStore not set
	EXPECT_THROW( persister->restoreRevision(1), co::IllegalStateException ); //spaceStore not set

	co::RefPtr<co::IObject> spaceStoreObj = co::newInstance( "ca.DBSpaceStore" );
	spaceStoreObj->getService<ca::INamed>()->setName( fileName );

	persisterObj->setService( "store", spaceStoreObj->getService<ca::ISpaceStore>() );

	remove( fileName.c_str() );
	sqlite3* hndl;
	sqlite3_open( fileName.c_str(), &hndl );
	sqlite3_close( hndl );

	EXPECT_THROW( persister->restoreRevision(1), co::IllegalArgumentException ); //empty database

}

TEST_F( SpacePersisterTest, testNewFileSetup )
{
	startWithExtendedERM();
	_relAB->setMultiplicityB( mult( 1, 2 ) );
	_relBC->setMultiplicityA( mult( 3, 4 ) );
	_relBC->setMultiplicityB( mult( 5, 6 ) );
	_relCA->setMultiplicityA( mult( 7, 8 ) );
	_relCA->setMultiplicityB( mult( 9, 0 ) );

	co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	universeObj->setService("model", _model.get());
	ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

	std::string fileName = "SimpleSpaceSave.db";
	
	remove( fileName.c_str() );

	co::RefPtr<co::IObject> persisterObj = co::newInstance( "ca.SpacePersister" );
	ca::ISpacePersister* persister = persisterObj->getService<ca::ISpacePersister>();
	
	co::RefPtr<co::IObject> spaceFileObj = co::newInstance( "ca.DBSpaceStore" );
	spaceFileObj->getService<ca::INamed>()->setName( fileName );

	persisterObj->setService( "store", spaceFileObj->getService<ca::ISpaceStore>() );
	persisterObj->setService( "universe", universe );

	EXPECT_NO_THROW( persister->initialize( _erm->getProvider() ) );
	EXPECT_NO_THROW( persister->restoreRevision(1) );

	ca::ISpace * spaceRestored = persister->getSpace();
	
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
	EXPECT_EQ( "relation A-B", rel->getRelation() );
	EXPECT_EQ( entities[0], rel->getEntityA() );
	EXPECT_EQ( entities[1], rel->getEntityB() );
	EXPECT_EQ( 0, rel->getMultiplicityA().min );
	EXPECT_EQ( 0, rel->getMultiplicityA().max );
	EXPECT_EQ( 1, rel->getMultiplicityB().min );
	EXPECT_EQ( 2, rel->getMultiplicityB().max );

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

