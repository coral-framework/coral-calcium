/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "../ERMSpace.h"
#include <ca/INamed.h>
#include <ca/IArchive.h>
#include <ca/IOException.h>
#include <ca/ModelException.h>

class LuaArchiveTests : public ERMSpace {};

TEST_F( LuaArchiveTests, setup )
{
	co::RefPtr<co::IObject> object = co::newInstance( "ca.LuaArchive" );

	ca::IArchive* archive = object->getService<ca::IArchive>();
	ASSERT_TRUE( archive != NULL );

	// expect ModelExceptions since we have not set a model
	EXPECT_THROW( archive->save( object.get() ), ca::ModelException );
	EXPECT_THROW( archive->restore(), ca::ModelException );

	EXPECT_NO_THROW( object->setService( "model", _model.get() ) );
	EXPECT_EQ( _model.get(), object->getService( "model" ) );

	// expect IOExceptions since we have not set a file name
	EXPECT_THROW( archive->save( object.get() ), ca::IOException );
	EXPECT_THROW( archive->restore(), ca::IOException );

	ca::INamed* archiveFile = object->getService<ca::INamed>();
	ASSERT_TRUE( archiveFile != NULL );
	EXPECT_EQ( "", archiveFile->getName() );

	EXPECT_NO_THROW( archiveFile->setName( "LuaArchiveTest.lua" ) );
	EXPECT_EQ( "LuaArchiveTest.lua", archiveFile->getName() );

	// now the LuaArchive is properly configured

	// calling restore() should fail because the file does not exist
	EXPECT_THROW( archive->restore(), ca::IOException );

	// calling save() with an undefined object type should raise a ModelException
	EXPECT_THROW( archive->save( object.get() ), ca::ModelException );
}

inline erm::Multiplicity mult( co::int32 min, co::int32 max )
{
	erm::Multiplicity m;
	m.min = min; m.max = max;
	return m;
}

TEST_F( LuaArchiveTests, simpleSaveRestore )
{
	// setup an ERM
	startWithExtendedERM();

	_relAB->setMultiplicityB( mult( 1, 2 ) );
	_relBC->setMultiplicityA( mult( 3, 4 ) );
	_relBC->setMultiplicityB( mult( 5, 6 ) );
	_relCA->setMultiplicityA( mult( 7, 8 ) );
	_relCA->setMultiplicityB( mult( 9, 0 ) );

	// save our ERM
	co::RefPtr<co::IObject> object = co::newInstance( "ca.LuaArchive" );
	object->setService( "model", _model.get() );
	object->getService<ca::INamed>()->setName( "SimpleSaveRestoreTest.lua" );

	ca::IArchive* archive = object->getService<ca::IArchive>();

	EXPECT_NO_THROW( archive->save( _erm->getProvider() ) );

	// restore the ERM
	co::RefPtr<co::IObject> restoredObj;
	try { restoredObj = archive->restore(); }
	catch( std::exception& e ) { printf("E: %s\n", e.what() ); }

	ASSERT_TRUE( restoredObj.isValid() );

	// check the restored state
	erm::IModel* erm = restoredObj->getService<erm::IModel>();
	ASSERT_TRUE( erm != NULL );

	co::Range<erm::IEntity*> entities = erm->getEntities();
	ASSERT_EQ( 3, entities.getSize() );

	EXPECT_EQ( "Entity A", entities[0]->getName() );
	EXPECT_EQ( "Entity B", entities[1]->getName() );
	EXPECT_EQ( "Entity C", entities[2]->getName() );

	co::Range<erm::IRelationship*> rels = erm->getRelationships();
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
