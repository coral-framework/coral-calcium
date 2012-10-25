/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _domSPACE_H_
#define _domSPACE_H_

#include <gtest/gtest.h>

#include <co/Coral.h>
#include <co/IField.h>
#include <co/IObject.h>

#include <ca/IModel.h>
#include <ca/ISpace.h>
#include <ca/IUniverse.h>

#include <dom/ICompany.h>
#include <dom/IDeveloper.h>
#include <dom/IManager.h>
#include <dom/IProject.h>

class CompanySpace : public ::testing::Test
{
public:
	virtual ~CompanySpace() {;}

protected:

	void SetUp();
	void TearDown();

	co::IObject* createCompanyGraph();

protected:
	std::string _modelName;

	co::IObjectRef _modelObj;
	co::IObjectRef _universeObj;
	co::IObjectRef _spaceObj;

	ca::IModelRef _model;
	ca::IUniverseRef _universe;
	ca::ISpaceRef _space;

	dom::ICompanyRef _company;
	dom::IProjectRef _software;
	dom::IProjectRef _dataMaintain;

	dom::IManagerRef _manager1;
	dom::IManagerRef _manager2;

	dom::IDeveloperRef _developer1;
	dom::IDeveloperRef _developer2;

	dom::IDeveloperRef _developer3;
	dom::IDeveloperRef _developer4;

};

#endif // _ERMSPACE_H_
