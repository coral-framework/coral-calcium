/*
 * Calcium Object Model Framework
 * See copyright notice in LICENSE.md
 */

#include <gtest/gtest.h>

#include <co/Type.h>
#include <co/Coral.h>
#include <co/Component.h>
#include <co/AttributeInfo.h>
#include <co/InterfaceInfo.h>
#include <co/IllegalArgumentException.h>
#include <ca/IObjectModel.h>

ca::IObjectModel* loadModel( const std::string& name )
{
	static co::RefPtr<ca::IObjectModel> s_model;
	co::RefPtr<co::Component> comp = co::newInstance( "ca.ObjectModel" );
	s_model = comp->getFacet<ca::IObjectModel>();
	s_model->setName( name );
	return s_model.get();
}

class TypeMismatch {};

template<typename T>
bool containsMemberOfType( co::ArrayRange<T* const> range, const std::string& memberName, const std::string& typeName )
{
	for( ; range; range.popFirst() )
		if( range.getFirst()->getName() == memberName )
			if( range.getFirst()->getType()->getFullName() == typeName )
				return true;
			else
				throw TypeMismatch();
	return false;
}

#define ASSERT_EXCEPTION( code, expectedErrorMsg ) \
	try { code; FAIL() << "an exception should have been raised"; } \
	catch( const std::exception& e ) { std::string raisedMsg( e.what() ); \
		if( raisedMsg.find( expectedErrorMsg ) == std::string::npos ) \
			FAIL()	<< "raised message (\"" << raisedMsg << "\") does not contain the expected message (\"" \
					<< expectedErrorMsg << "\")"; \
	}

#define ASSERT_MODEL_ERROR( modelName, type, expectedErrorMsg ) \
	ASSERT_EXCEPTION( loadModel( modelName )->contains( type ), expectedErrorMsg );


TEST( ObjectModelTests, simpleValidModels )
{
	co::Type* someEnum = co::getType( "camodels.SomeEnum" );
	co::Type* someInterface = co::getType( "camodels.SomeInterface" );
	co::Type* someStruct = co::getType( "camodels.SomeStruct" );

	ca::IObjectModel* model = loadModel( "valid1" );
	ASSERT_TRUE( model->contains( someEnum ) );
	ASSERT_FALSE( model->contains( someInterface ) );

	model = loadModel( "valid2" );
	ASSERT_TRUE( model->contains( someInterface ) );
	ASSERT_FALSE( model->contains( someStruct ) );

	// contains() does not respond to primitive types and arrays
	EXPECT_FALSE( model->contains( co::getType( "string" ) ) );
	EXPECT_FALSE( model->contains( co::getType( "string[]" ) ) );
	EXPECT_FALSE( model->contains( co::getType( "camodels.SomeInterface[]" ) ) );

	model = loadModel( "valid3" );
	ASSERT_FALSE( model->alreadyContains( someEnum ) );
	ASSERT_FALSE( model->alreadyContains( someInterface ) );
	ASSERT_FALSE( model->alreadyContains( someStruct ) );
	ASSERT_TRUE( model->contains( someEnum ) );
	ASSERT_TRUE( model->alreadyContains( someEnum ) );
	ASSERT_TRUE( model->alreadyContains( someInterface ) );
	ASSERT_TRUE( model->alreadyContains( someStruct ) );
	ASSERT_TRUE( model->contains( someInterface ) );
	ASSERT_TRUE( model->contains( someStruct ) );

	EXPECT_THROW( model->getAttributes( someEnum ), co::IllegalArgumentException );

	EXPECT_EQ( 4, model->getAttributes( someInterface ).getSize() );
	EXPECT_TRUE( containsMemberOfType( model->getAttributes( someInterface ), "str1", "string" ) );
	EXPECT_TRUE( containsMemberOfType( model->getAttributes( someInterface ), "enum1", "camodels.SomeEnum" ) );
	EXPECT_TRUE( containsMemberOfType( model->getAttributes( someInterface ), "struct1", "camodels.SomeStruct" ) );
	EXPECT_TRUE( containsMemberOfType( model->getAttributes( someInterface ), "strArray", "string[]" ) );
	EXPECT_FALSE( containsMemberOfType( model->getAttributes( someInterface ), "any1", "any" ) );
	EXPECT_THROW( containsMemberOfType( model->getAttributes( someInterface ), "str1", "int32" ), TypeMismatch );

	EXPECT_EQ( 4, model->getAttributes( someStruct ).getSize() );
	EXPECT_TRUE( containsMemberOfType( model->getAttributes( someStruct ), "int1", "int32" ) );
	EXPECT_TRUE( containsMemberOfType( model->getAttributes( someStruct ), "str1", "string" ) );
	EXPECT_TRUE( containsMemberOfType( model->getAttributes( someStruct ), "enum1", "camodels.SomeEnum" ) );
	EXPECT_TRUE( containsMemberOfType( model->getAttributes( someStruct ), "interface1", "camodels.SomeInterface" ) );
	EXPECT_FALSE( containsMemberOfType( model->getAttributes( someStruct ), "whatever", "bool" ) );
	EXPECT_THROW( containsMemberOfType( model->getAttributes( someStruct ), "int1", "uint32" ), TypeMismatch );
}

TEST( ObjectModelTests, simpleInvalidModels )
{
	co::Type* someEnum = co::getType( "camodels.SomeEnum" );
	co::Type* someInterface = co::getType( "camodels.SomeInterface" );
	co::Type* someStruct = co::getType( "camodels.SomeStruct" );

	ASSERT_MODEL_ERROR( "invalid1", someEnum, "does not exist" );
	ASSERT_MODEL_ERROR( "invalid2", someInterface, "type 'oops.DoesNotExist' does not exist" );
	ASSERT_MODEL_ERROR( "invalid3", someStruct, "no such attribute 'oops' in type 'camodels.SomeInterface'" );

	ASSERT_MODEL_ERROR( "invalid4", someEnum, "attribute 'str1' of type 'camodels.SomeInterface' was expected "
							"to be of type 'oops.DoesNotExist', but it is really of type 'string'" );

	ASSERT_MODEL_ERROR( "invalid5", someEnum, "in member 'enum1' of type 'camodels.SomeInterface': "
							"type 'camodels.SomeEnum' is not in the object model" );

	ASSERT_MODEL_ERROR( "invalid6", someEnum, "enum type 'camodels.SomeEnum' should not have a field list" );
}

TEST( ObjectModelTests, extraModuleDefinitions )
{
	/*
		Model 'extraModule' has duplicate definitions in modules 'camodels' and 'erm'.
		Both modules define the type 'erm.IEntity'. Also, module 'erm' defines the type
		'camodels.SomeStruct', which is not defined by module 'camodels'.
	 */
	co::Type* someEnum = co::getType( "camodels.SomeEnum" );
	co::Type* someStruct = co::getType( "camodels.SomeStruct" );
	co::Type* ermIEntity = co::getType( "erm.IEntity" );
	co::Type* ermIModel = co::getType( "erm.IModel" );

	/*
		If we query a type from 'camodels' first, the duplicates should only be detected
		when we query an unknown type from module 'erm' later.
	 */
	ca::IObjectModel* model = loadModel( "extraModule" );
	ASSERT_TRUE( model->contains( someEnum ) );
	EXPECT_TRUE( model->alreadyContains( ermIEntity ) );
	ASSERT_FALSE( model->contains( someStruct ) );
	ASSERT_EXCEPTION( model->contains( ermIModel ), "type 'erm.IEntity' is already in the object model" );

	/*
		If we query a type from 'erm' first, the duplicates should only be detected when
		we query an unknown type from module 'camodels' later. And this time, the model should
		contain the type 'camodels.SomeStruct', which is only defined by module 'erm'.
	 */
	model = loadModel( "extraModule" );
	ASSERT_TRUE( model->contains( ermIModel ) );
	EXPECT_TRUE( model->alreadyContains( ermIEntity ) );
	EXPECT_TRUE( model->alreadyContains( someStruct ) );
	ASSERT_EXCEPTION( model->contains( someEnum ), "type 'erm.IEntity' is already in the object model" );
}

TEST( ObjectModelTests, interModuleDefinitions )
{
	/*
		Model 'erm' is collaboratively defined by two modules, 'erm' and 'camodels'.
		This time each module only defines their own types and we have no problem.
	 */
	ca::IObjectModel* model = loadModel( "erm" );
	ASSERT_TRUE( model->contains( co::getType( "erm.IEntity" ) ) );
	ASSERT_TRUE( model->contains( co::getType( "erm.Entity" ) ) );
	ASSERT_TRUE( model->contains( co::getType( "camodels.SomeStruct" ) ) );
	ASSERT_FALSE( model->contains( co::getType( "co.System" ) ) );
}
