/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "TestUtils.h"

#include <co/Coral.h>
#include <co/IEnum.h>
#include <co/IPort.h>
#include <co/IField.h>
#include <co/IObject.h>
#include <co/IStruct.h>
#include <co/IComponent.h>
#include <co/IInterface.h>
#include <co/IllegalArgumentException.h>
#include <ca/IModel.h>

ca::IModel* loadModel( const std::string& name )
{
	co::IObject* object = co::newInstance( "ca.Model" );
	ca::IModel* model = object->getService<ca::IModel>();
	model->setName( name );
	return model;
}

class TypeMismatch {};

template<typename T>
bool containsMemberOfType( co::RefVector<T> vec, const std::string& memberName, const std::string& typeName )
{
	size_t count = vec.size();
	for( size_t i = 0; i < count; ++i )
		if( vec[i]->getName() == memberName )
			if( vec[i]->getType()->getFullName() == typeName )
				return true;
			else
				throw TypeMismatch();
	return false;
}

#define ASSERT_MODEL_ERROR( modelName, type, expectedErrorMsg ) \
	{ co::RefPtr<ca::IModel> m = loadModel( modelName ); \
		ASSERT_EXCEPTION( m->contains( type ), expectedErrorMsg ); }

TEST( ModelTests, simpleValidModels )
{
	co::IEnum* someEnum = co::cast<co::IEnum>( co::getType( "camodels.SomeEnum" ) );
	co::IInterface* someInterface = co::cast<co::IInterface>( co::getType( "camodels.SomeInterface" ) );
	co::IStruct* someStruct = co::cast<co::IStruct>( co::getType( "camodels.SomeStruct" ) );

	co::RefPtr<ca::IModel> model = loadModel( "valid1" );
	ASSERT_TRUE( model->contains( someEnum ) );
	ASSERT_FALSE( model->contains( someInterface ) );
	ASSERT_TRUE( model->getUpdates().getSize() == 0 );

	model = loadModel( "valid2" );

	ASSERT_TRUE( model->getUpdates().getSize() == 0 );

	ASSERT_TRUE( model->contains( someInterface ) );
	ASSERT_FALSE( model->contains( someStruct ) );

	// contains() does not respond to primitive types and arrays
	EXPECT_FALSE( model->contains( co::getType( "string" ) ) );
	EXPECT_FALSE( model->contains( co::getType( "string[]" ) ) );
	EXPECT_FALSE( model->contains( co::getType( "camodels.SomeInterface[]" ) ) );

	model = loadModel( "valid3" );
	
	ASSERT_TRUE( model->getUpdates().getSize() == 0 );

	ASSERT_FALSE( model->alreadyContains( someEnum ) );
	ASSERT_FALSE( model->alreadyContains( someInterface ) );
	ASSERT_FALSE( model->alreadyContains( someStruct ) );
	ASSERT_TRUE( model->contains( someEnum ) );
	ASSERT_TRUE( model->alreadyContains( someEnum ) );
	ASSERT_TRUE( model->alreadyContains( someInterface ) );
	ASSERT_TRUE( model->alreadyContains( someStruct ) );
	ASSERT_TRUE( model->contains( someInterface ) );
	ASSERT_TRUE( model->contains( someStruct ) );

	

	co::RefVector<co::IField> fields;

	model->getFields( someInterface, fields );
	EXPECT_EQ( 5, fields.size() );
	EXPECT_TRUE( containsMemberOfType( fields, "str1", "string" ) );
	EXPECT_TRUE( containsMemberOfType( fields, "enum1", "camodels.SomeEnum" ) );
	EXPECT_TRUE( containsMemberOfType( fields, "struct1", "camodels.SomeStruct" ) );
	EXPECT_TRUE( containsMemberOfType( fields, "strArray", "string[]" ) );
	EXPECT_TRUE( containsMemberOfType( fields, "autoRef", "camodels.SomeInterface" ) );
	EXPECT_FALSE( containsMemberOfType( fields, "any1", "any" ) );
	EXPECT_THROW( containsMemberOfType( fields, "str1", "int32" ), TypeMismatch );

	model->getFields( someStruct, fields );
	EXPECT_EQ( 3, fields.size() );
	EXPECT_TRUE( containsMemberOfType( fields, "int1", "int32" ) );
	EXPECT_TRUE( containsMemberOfType( fields, "str1", "string" ) );
	EXPECT_TRUE( containsMemberOfType( fields, "enum1", "camodels.SomeEnum" ) );
	EXPECT_FALSE( containsMemberOfType( fields, "interface1", "camodels.SomeInterface" ) );
	EXPECT_FALSE( containsMemberOfType( fields, "whatever", "bool" ) );
	EXPECT_THROW( containsMemberOfType( fields, "int1", "uint32" ), TypeMismatch );
}

TEST( ModelTests, testValidModelsWithUpdate )
{
	co::RefPtr<ca::IModel> model = loadModel( "valid4" );

	ASSERT_NO_THROW( model->contains( co::getType( "camodels.SomeEnum") ) );
	ASSERT_TRUE( model->getUpdates().getSize() == 1 );
	ASSERT_EQ( "script1.lua", model->getUpdates()[0] );

	model = loadModel( "valid5" );
	ASSERT_NO_THROW( model->contains( co::getType( "camodels.SomeEnum") ) );

	ASSERT_TRUE( model->getUpdates().getSize() == 4 );
	ASSERT_EQ( "script1.lua", model->getUpdates()[0] );
	ASSERT_EQ( "script2.lua", model->getUpdates()[1] );
	ASSERT_EQ( "script3.lua", model->getUpdates()[2] );	
	ASSERT_EQ( "script4.lua", model->getUpdates()[3] );


	//assure order of declaration

}

TEST( ModelTests, simpleInvalidModels )
{
	co::IType* someEnum = co::getType( "camodels.SomeEnum" );
	co::IType* someInterface = co::getType( "camodels.SomeInterface" );
	co::IType* someStruct = co::getType( "camodels.SomeStruct" );

	ASSERT_MODEL_ERROR( "invalid1", someEnum, "could not load type" );
	ASSERT_MODEL_ERROR( "invalid2", someInterface, "could not load type 'oops.DoesNotExist'" );
	ASSERT_MODEL_ERROR( "invalid3", someStruct, "no such field 'oops' in type 'camodels.SomeInterface'" );

	ASSERT_MODEL_ERROR( "invalid4", someEnum, "field 'str1' in type 'camodels.SomeInterface' was expected "
							"to be of type 'oops.DoesNotExist', but it is really of type 'string'" );

	ASSERT_MODEL_ERROR( "invalid5", someEnum, "in member 'enum1' of type 'camodels.SomeInterface': "
							"type 'camodels.SomeEnum' is not in the object model" );

	ASSERT_MODEL_ERROR( "invalid6", someEnum, "enum 'camodels.SomeEnum' should not have a field list" );

	ASSERT_MODEL_ERROR( "invalid7", someEnum, "array type 'camodels.SomeEnum[]' cannot be defined in the "
							"object model; define 'camodels.SomeEnum' instead" );

	ASSERT_MODEL_ERROR( "invalid8", someEnum, "exception type 'ca.ModelException' cannot be defined in the object model" );

	ASSERT_MODEL_ERROR( "invalid9", someEnum, "no such port 'invalidItf' in component 'ca.Space'" );

	ASSERT_MODEL_ERROR( "invalid10", someEnum, "port 'space' in component 'ca.Space' was expected "
							"to be of type 'camodels.SomeInterface', but it is really of type 'ca.ISpace'" );

	ASSERT_MODEL_ERROR( "invalid11", someEnum, "illegal read-only field 'fullName' defined for type 'co.IType'" );

	ASSERT_MODEL_ERROR( "invalid12", someEnum, "cannot define primitive type 'int32'" );

	ASSERT_MODEL_ERROR( "invalid13", someEnum, "illegal reference field 'interface1' in complex "
							"value type 'camodels.SomeStruct'" );

	ASSERT_MODEL_ERROR( "invalid14", someEnum, "illegal reference field 'interfaceArray' "
							"in complex value type 'camodels.SomeStruct'" );

	ASSERT_MODEL_ERROR( "invalid15", someEnum, "illegal script name" );

}

TEST( ModelTests, extraModuleDefinitions )
{
	/*
		Model 'extraModule' has duplicate definitions in modules 'camodels' and 'erm'.
		Both modules define the type 'erm.IEntity'. Also, module 'erm' defines the type
		'camodels.SomeStruct', which is not defined by module 'camodels'.
	 */
	co::IType* someEnum = co::getType( "camodels.SomeEnum" );
	co::IType* someStruct = co::getType( "camodels.SomeStruct" );
	co::IType* ermIEntity = co::getType( "erm.IEntity" );
	co::IType* ermIModel = co::getType( "erm.IModel" );

	/*
		If we query a type from 'camodels' first, the duplicates should only be detected
		when we query an unknown type from module 'erm' later.
	 */
	co::RefPtr<ca::IModel> model = loadModel( "extraModule" );
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

TEST( ModelTests, interModuleDefinitions )
{
	/*
		Model 'erm' is collaboratively defined by two modules, 'erm' and 'camodels'.
		This time each module only defines their own types and we have no problem.
	 */
	co::RefPtr<ca::IModel> model = loadModel( "erm" );
	ASSERT_TRUE( model->contains( co::getType( "erm.IEntity" ) ) );
	ASSERT_TRUE( model->contains( co::getType( "erm.Entity" ) ) );
	ASSERT_TRUE( model->contains( co::getType( "camodels.SomeStruct" ) ) );
	ASSERT_FALSE( model->contains( co::getType( "co.System" ) ) );
}
