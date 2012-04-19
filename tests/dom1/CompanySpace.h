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

	co::RefPtr<co::IObject> _modelObj;
	co::RefPtr<co::IObject> _universeObj;
	co::RefPtr<co::IObject> _spaceObj;

	co::RefPtr<ca::IModel> _model;
	co::RefPtr<ca::IUniverse> _universe;
	co::RefPtr<ca::ISpace> _space;

	co::RefPtr<dom::ICompany> _company;
	co::RefPtr<dom::IProject> _software;
	co::RefPtr<dom::IProject> _dataMaintain;

	co::RefPtr<dom::IManager> _manager1;
	co::RefPtr<dom::IManager> _manager2;

	co::RefPtr<dom::IDeveloper> _developer1;
	co::RefPtr<dom::IDeveloper> _developer2;

	co::RefPtr<dom::IDeveloper> _developer3;
	co::RefPtr<dom::IDeveloper> _developer4;

};

#endif // _ERMSPACE_H_
