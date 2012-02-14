#if 0

#include "../ERMSpace.h"
#include "persistence/SpaceUpdater.h"

#include <gtest/gtest.h>

#include <co/IObject.h>
#include <co/RefPtr.h>

#include <ca/INamed.h>
#include <ca/ISpacePersister.h>
#include <ca/ISpaceStore.h>
#include <ca/StoredType.h>


class SpaceUpdaterTests : public ERMSpace
{
public:

	void SetUp()
	{
		ERMSpace::SetUp();
		startWithExtendedERM();
	}

};

TEST_F( SpaceUpdaterTests, testCheckModel )
{
	
	co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	universeObj->setService("model", _model.get());

	ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

	co::IObject* persisterObj = co::newInstance( "ca.SpacePersister" );
	co::RefPtr<ca::ISpacePersister> persister = persisterObj->getService<ca::ISpacePersister>();

	co::RefPtr<co::IObject> storeObj = co::newInstance( "ca.SQLiteSpaceStore" );
	remove( "dbFile.sql" );
	storeObj->getService<ca::INamed>()->setName( "dbFile.sql" );

	ca::ISpaceStore* store = storeObj->getService<ca::ISpaceStore>();

	persisterObj->setService( "store", store );
	persisterObj->setService( "universe", universe );

	persister->initialize( _erm->getProvider() );

	
	co::RefPtr<co::IObject> changedModel = co::newInstance( "ca.Model" );
	changedModel->getService<ca::IModel>()->setName( "ermNew" );

	co::RefPtr<co::IObject> scrambledModel = co::newInstance( "ca.Model" );
	scrambledModel->getService<ca::IModel>()->setName( "ermScrambled" );

	store->open();
	co::uint32 typeId = store->getOrAddType( "erm.IModel", 1 );

	ca::StoredType storedType;
	
	store->getType( typeId, storedType );
	store->close();

	ca::SpaceUpdater spaceUpdater( store );
	store->open();
	EXPECT_TRUE( spaceUpdater.checkModelChange( changedModel->getService<ca::IModel>(), storedType ) );
	store->close();

	store->open();
	EXPECT_FALSE( spaceUpdater.checkModelChange( _model.get(), storedType ) );
	store->close();

	store->open();
	EXPECT_FALSE( spaceUpdater.checkModelChange( scrambledModel->getService<ca::IModel>(), storedType ) );
	store->close();

	//component

	store->open();
	typeId = store->getOrAddType( "erm.Relationship", 1 );

	store->getType( typeId, storedType );
	store->close();

	store->open();
	EXPECT_TRUE( spaceUpdater.checkModelChange( changedModel->getService<ca::IModel>(), storedType ) );
	store->close();

	store->open();
	EXPECT_FALSE( spaceUpdater.checkModelChange( _model.get(), storedType ) );
	store->close();

	store->open();
	EXPECT_FALSE( spaceUpdater.checkModelChange( scrambledModel->getService<ca::IModel>(), storedType ) );
	store->close();

}

#endif
