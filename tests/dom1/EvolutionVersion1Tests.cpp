/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */
#include "CompanySpace.h"

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

#include <dom/ICompany.h>
#include <dom/IDeveloper.h>
#include <dom/IManager.h>
#include <dom/IProject.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/INamed.h>
#include <ca/IUniverse.h>
#include <ca/ModelException.h>
#include <ca/NoSuchObjectException.h>
#include <ca/IOException.h>
#include <ca/ISpaceStore.h>
#include <cstdio>


class Evolution1Version1Tests : public CompanySpace 
{
public:

	void SetUp()
	{
		CompanySpace::SetUp();
	}

	co::RefPtr<ca::ISpacePersister> createPersister( const std::string& fileName )
	{
		co::IObject* persisterObj = co::newInstance( "ca.SpacePersister" );
		co::RefPtr<ca::ISpacePersister> persister = persisterObj->getService<ca::ISpacePersister>();

		co::RefPtr<co::IObject> spaceFileObj = co::newInstance( "ca.SQLiteSpaceStore" );
		spaceFileObj->getService<ca::INamed>()->setName( fileName );

			persisterObj->setService( "store", spaceFileObj->getService<ca::ISpaceStore>() );
		persisterObj->setService( "universe", _universe.get() );

		return persister;
	}

};

TEST_F( Evolution1Version1Tests, pass1CreateCompanyV1File )
{
	std::string fileName = "CompanyV1.db";

	remove( fileName.c_str() );

	co::RefPtr<ca::ISpacePersister> persister = createPersister( fileName );

	ASSERT_NO_THROW( persister->initialize( _company->getProvider() ) );

	co::RefPtr<ca::ISpacePersister> persisterToRestore = createPersister( fileName );

	ASSERT_NO_THROW( persisterToRestore->restoreRevision( 1 ) );

	ca::ISpace * spaceRestored = persisterToRestore->getSpace();
	
	co::IObject* objRest = spaceRestored->getRootObject();

	dom::ICompany* company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	co::Range<dom::IProject* const> projects = company->getProjects();
	ASSERT_EQ( 2, projects.getSize() );

	EXPECT_EQ( "Software2.0", projects[0]->getName() );
	EXPECT_EQ( "Software1.0 Maintenance", projects[1]->getName() );

	EXPECT_EQ( 1000000, projects[0]->getEarnings() );
	EXPECT_EQ( 50000, projects[1]->getEarnings() );

	EXPECT_EQ( false, projects[0]->getIsService() );
	EXPECT_EQ( true, projects[1]->getIsService() );

	co::Range<dom::IDeveloper* const> devs = projects[0]->getDevelopers();
	ASSERT_EQ( 2, devs.getSize() );

	EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
	EXPECT_EQ( 1000, devs[0]->getSalary() );
	EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
	EXPECT_EQ( 4000, devs[1]->getSalary() );

	//forcing one more revision
	devs[1]->setSalary( 5000 );
	spaceRestored->addChange( devs[1] );
	spaceRestored->notifyChanges();

	dom::IManager* manager = projects[0]->getManager();

	EXPECT_EQ( "Richard Scrum Master", manager->getName() );
	EXPECT_EQ( 10000, manager->getSalary() );

	devs = projects[1]->getDevelopers();
	ASSERT_EQ( 2, devs.getSize() );

	EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
	EXPECT_EQ( 5000, devs[0]->getSalary() );
	EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
	EXPECT_EQ( 3000, devs[1]->getSalary() );

	manager = projects[1]->getManager();

	EXPECT_EQ( "Wiliam Kanban Expert", manager->getName() );
	EXPECT_EQ( 9000, manager->getSalary() );

	ASSERT_NO_THROW( persisterToRestore->save() );

}

TEST_F( Evolution1Version1Tests, pass2restoreFromEvolvedCompanyFileValidRevision ) // assure the bd file still valid
{
	std::string fileName = "CompanyV1.db";

	co::RefPtr<ca::ISpacePersister> persisterToRestore = createPersister( fileName );

	ASSERT_NO_THROW( persisterToRestore->restoreRevision( 2 ) );

	ca::ISpace * spaceRestored = persisterToRestore->getSpace();

	co::IObject* objRest = spaceRestored->getRootObject();

	dom::ICompany* company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	co::Range<dom::IProject* const> projects = company->getProjects();
	ASSERT_EQ( 2, projects.getSize() );

	EXPECT_EQ( "Software2.0", projects[0]->getName() );
	EXPECT_EQ( "Software1.0 Maintenance", projects[1]->getName() );

	EXPECT_EQ( 1000000, projects[0]->getEarnings() );
	EXPECT_EQ( 50000, projects[1]->getEarnings() );

	EXPECT_EQ( false, projects[0]->getIsService() );
	EXPECT_EQ( true, projects[1]->getIsService() );

	co::Range<dom::IDeveloper* const> devs = projects[0]->getDevelopers();
	ASSERT_EQ( 2, devs.getSize() );

	EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
	EXPECT_EQ( 1000, devs[0]->getSalary() );
	EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
	EXPECT_EQ( 5000, devs[1]->getSalary() );

	dom::IManager* manager = projects[0]->getManager();

	EXPECT_EQ( "Richard Scrum Master", manager->getName() );
	EXPECT_EQ( 10000, manager->getSalary() );

	devs = projects[1]->getDevelopers();
	ASSERT_EQ( 2, devs.getSize() );

	EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
	EXPECT_EQ( 5000, devs[0]->getSalary() );
	EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
	EXPECT_EQ( 3000, devs[1]->getSalary() );

	manager = projects[1]->getManager();

	EXPECT_EQ( "Wiliam Kanban Expert", manager->getName() );
	EXPECT_EQ( 9000, manager->getSalary() );

}

TEST_F( Evolution1Version1Tests, pass2restoreFromEvolvedCompanyFileInvalidRevision ) // can not restore revision from future version (yet)
{
	std::string fileName = "CompanyV1.db";

	co::RefPtr<ca::ISpacePersister> persisterToRestore = createPersister( fileName );

	ASSERT_THROW( persisterToRestore->restore(), ca::IOException );
}
