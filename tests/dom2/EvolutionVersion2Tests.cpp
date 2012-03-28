/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

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
#include <dom/IEmployee.h>
#include <dom/IProduct.h>
#include <dom/IService.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/INamed.h>
#include <ca/IUniverse.h>
#include <ca/ModelException.h>
#include <ca/NoSuchObjectException.h>
#include <ca/IOException.h>
#include <ca/ISpaceStore.h>

#include <cstdio>


class EvolutionVersion2Tests : public ::testing::Test
{
public:

	void SetUp()
	{
		
	}

	co::RefPtr<ca::ISpacePersister> createPersister( const std::string& fileName, const std::string& modelName )
	{
		co::IObject* persisterObj = co::newInstance( "ca.SpacePersister" );
		co::RefPtr<ca::ISpacePersister> persister = persisterObj->getService<ca::ISpacePersister>();

		co::RefPtr<co::IObject> spaceFileObj = co::newInstance( "ca.SQLiteSpaceStore" );
		spaceFileObj->getService<ca::INamed>()->setName( fileName );

		co::IObject* universeObj = co::newInstance( "ca.Universe" );

		co::IObject* modelObj = co::newInstance( "ca.Model" );
		ca::IModel* model = modelObj->getService<ca::IModel>();
		model->setName( modelName );


		_universe = universeObj->getService<ca::IUniverse>();

		universeObj->setService( "model", model );

		persisterObj->setService( "store", spaceFileObj->getService<ca::ISpaceStore>() );
		persisterObj->setService( "universe", _universe.get() );
		
		return persister;
	}

	co::RefPtr<ca::IUniverse> _universe;

};

TEST_F( EvolutionVersion2Tests, testScriptNotFound )
{
	std::string fileName = "CompanyV1.db";

	co::RefPtr<ca::ISpacePersister> persister = createPersister( fileName, "notfound" );

	ASSERT_THROW( persister->restore(), ca::IOException );

}

TEST_F( EvolutionVersion2Tests, testSyntaxErrorUpdate )
{
	std::string fileName = "CompanyV1.db";

	co::RefPtr<ca::ISpacePersister> persister = createPersister( fileName, "syntaxerror" );

	ASSERT_THROW( persister->restore(), ca::IOException );
}

TEST_F( EvolutionVersion2Tests, testScriptWithoutUpdateFunc )
{
	std::string fileName = "CompanyV1.db";

	co::RefPtr<ca::ISpacePersister> persister = createPersister( fileName, "scriptNoUpdate" );

	ASSERT_THROW( persister->restore(), ca::IOException );
}

TEST_F( EvolutionVersion2Tests, restoreV2SpaceFromV1FilePreviousRevision )
{
	std::string fileName = "CompanyV1.db";

	co::RefPtr<ca::ISpacePersister> persister = createPersister( fileName, "dom" );

	ASSERT_NO_THROW( persister->restoreRevision( 1 ) ); // restore allowed

	ca::ISpace * spaceRestored = persister->getSpace();

	co::IObject* objRest = spaceRestored->getRootObject();

	dom::ICompany* company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	co::Range<dom::IProduct* const> products = company->getProducts();
	ASSERT_EQ( 1, products.getSize() );

	EXPECT_EQ( "Software2.0", products[0]->getName() );
	EXPECT_EQ( 1000000, products[0]->getValue() );

	co::Range<dom::IEmployee* const> devs = products[0]->getDevelopers();

	EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
	EXPECT_EQ( 1000, devs[0]->getSalary() );
	EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
	EXPECT_EQ( 4000, devs[1]->getSalary() );

	EXPECT_EQ( "Developer", devs[0]->getRole() );
	EXPECT_EQ( "Developer", devs[1]->getRole() );

	dom::IEmployee* manager = products[0]->getLeader();

	EXPECT_EQ( "Richard Scrum Master", manager->getName() );
	EXPECT_EQ( 10000, manager->getSalary() );

	EXPECT_EQ( "Manager", manager->getRole() );

	co::Range<dom::IService* const> services = company->getServices();
	ASSERT_EQ( 1, services.getSize() );

	EXPECT_EQ( "Software1.0 Maintenance", services[0]->getName() );
	EXPECT_EQ( 50000, services[0]->getMonthlyIncome() );

	devs = services[0]->getMantainers();

	ASSERT_EQ( 2, devs.getSize() );

	EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
	EXPECT_EQ( 5000, devs[0]->getSalary() );
	EXPECT_EQ( "Developer", devs[0]->getRole() );

	EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
	EXPECT_EQ( 3000, devs[1]->getSalary() );
	EXPECT_EQ( "Developer", devs[1]->getRole() );

	ASSERT_THROW( persister->save(), ca::IOException ); //save not allowed
}

TEST_F( EvolutionVersion2Tests, restoreV2SpaceFromV1FileLastRevision )
{
	std::string fileName = "CompanyV1.db";

	co::RefPtr<ca::ISpacePersister> persister = createPersister( fileName, "dom" );

	ASSERT_NO_THROW( persister->restore() );

	ca::ISpace * spaceRestored = persister->getSpace();
	
	co::IObject* objRest = spaceRestored->getRootObject();

	dom::ICompany* company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	co::Range<dom::IProduct* const> products = company->getProducts();
	ASSERT_EQ( 1, products.getSize() );

	EXPECT_EQ( "Software2.0", products[0]->getName() );
	EXPECT_EQ( 1000000, products[0]->getValue() );

	co::Range<dom::IEmployee* const> devs = products[0]->getDevelopers();

	EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
	EXPECT_EQ( 1000, devs[0]->getSalary() );
	EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
	EXPECT_EQ( 5000, devs[1]->getSalary() );

	EXPECT_EQ( "Developer", devs[0]->getRole() );
	EXPECT_EQ( "Developer", devs[1]->getRole() );

	dom::IEmployee* manager = products[0]->getLeader();

	EXPECT_EQ( "Richard Scrum Master", manager->getName() );
	EXPECT_EQ( 10000, manager->getSalary() );

	EXPECT_EQ( "Manager", manager->getRole() );

	co::Range<dom::IService* const> services = company->getServices();
	ASSERT_EQ( 1, services.getSize() );

	EXPECT_EQ( "Software1.0 Maintenance", services[0]->getName() );
	EXPECT_EQ( 50000, services[0]->getMonthlyIncome() );

	devs = services[0]->getMantainers();

	ASSERT_EQ( 2, devs.getSize() );


	EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
	EXPECT_EQ( 5000, devs[0]->getSalary() );
	EXPECT_EQ( "Developer", devs[0]->getRole() );

	EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
	EXPECT_EQ( 3000, devs[1]->getSalary() );
	EXPECT_EQ( "Developer", devs[1]->getRole() );

	ASSERT_NO_THROW( persister->save() ); //save allowed

	co::RefPtr<ca::ISpacePersister> persisterToRestore = createPersister( fileName, "dom" );

	ASSERT_NO_THROW( persisterToRestore->restore() );

	spaceRestored = persisterToRestore->getSpace();

	objRest = spaceRestored->getRootObject();

	company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	products = company->getProducts();
	
	ASSERT_EQ( 1, products.getSize() );

	EXPECT_EQ( "Software2.0", products[0]->getName() );
	EXPECT_EQ( 1000000, products[0]->getValue() );

	devs = products[0]->getDevelopers();

	EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
	EXPECT_EQ( 1000, devs[0]->getSalary() );
	EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
	EXPECT_EQ( 5000, devs[1]->getSalary() );

	EXPECT_EQ( "Developer", devs[0]->getRole() );
	EXPECT_EQ( "Developer", devs[1]->getRole() );

	manager = products[0]->getLeader();

	EXPECT_EQ( "Richard Scrum Master", manager->getName() );
	EXPECT_EQ( 10000, manager->getSalary() );

	EXPECT_EQ( "Manager", manager->getRole() );
	manager->setRole("Development Manager");
	spaceRestored->addChange( manager );
	spaceRestored->notifyChanges();

	services = company->getServices();
	ASSERT_EQ( 1, services.getSize() );

	EXPECT_EQ( "Software1.0 Maintenance", services[0]->getName() );
	EXPECT_EQ( 50000, services[0]->getMonthlyIncome() );

	devs = services[0]->getMantainers();

	ASSERT_EQ( 2, devs.getSize() );

	EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
	EXPECT_EQ( 5000, devs[0]->getSalary() );
	EXPECT_EQ( "Developer", devs[0]->getRole() );

	EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
	EXPECT_EQ( 3000, devs[1]->getSalary() );
	EXPECT_EQ( "Developer", devs[1]->getRole() );

	devs[1]->setSalary( 4000 );
	spaceRestored->addChange( devs[1] );
	spaceRestored->notifyChanges();

	ASSERT_NO_THROW( persisterToRestore->save() );

	co::RefPtr<ca::ISpacePersister> persisterToRestore2 = createPersister( fileName, "dom" );

	persisterToRestore2->restore() ;

	spaceRestored = persisterToRestore2->getSpace();

	objRest = spaceRestored->getRootObject();

	company = objRest->getService<dom::ICompany>();
	ASSERT_TRUE( company != NULL );

	products = company->getProducts();

	ASSERT_EQ( 1, products.getSize() );

	EXPECT_EQ( "Software2.0", products[0]->getName() );
	EXPECT_EQ( 1000000, products[0]->getValue() );

	devs = products[0]->getDevelopers();

	EXPECT_EQ( "Joseph Java Newbie", devs[0]->getName() );
	EXPECT_EQ( 1000, devs[0]->getSalary() );
	EXPECT_EQ( "Michael CSharp Senior", devs[1]->getName() );
	EXPECT_EQ( 5000, devs[1]->getSalary() );

	EXPECT_EQ( "Developer", devs[0]->getRole() );
	EXPECT_EQ( "Developer", devs[1]->getRole() );

	manager = products[0]->getLeader();

	EXPECT_EQ( "Richard Scrum Master", manager->getName() );
	EXPECT_EQ( 10000, manager->getSalary() );

	EXPECT_EQ( "Development Manager", manager->getRole() );

	services = company->getServices();
	ASSERT_EQ( 1, services.getSize() );

	EXPECT_EQ( "Software1.0 Maintenance", services[0]->getName() );
	EXPECT_EQ( 50000, services[0]->getMonthlyIncome() );

	devs = services[0]->getMantainers();

	ASSERT_EQ( 2, devs.getSize() );

	EXPECT_EQ( "John Cplusplus Experienced", devs[0]->getName() );
	EXPECT_EQ( 5000, devs[0]->getSalary() );
	EXPECT_EQ( "Developer", devs[0]->getRole() );

	EXPECT_EQ( "Jacob Lua Junior", devs[1]->getName() );
	EXPECT_EQ( 4000, devs[1]->getSalary() );
	EXPECT_EQ( "Developer", devs[1]->getRole() );
}

