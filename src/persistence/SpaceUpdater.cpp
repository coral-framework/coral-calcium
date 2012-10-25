#if 0
#include "SpaceUpdater.h"

#include <co/Coral.h>
#include <co/IService.h>
#include <ca/ISpaceStore.h>
#include <co/IObject.h>
#include <co/IField.h>
#include <co/IPort.h>
#include <co/Log.h>

#include <co/IComponent.h>
#include <co/IInterface.h>
#include <co/IType.h>
#include <co/IArray.h>


#include <ca/IModel.h>

#include <vector>

namespace ca {

	bool SpaceUpdater::checkModelChange( ca::IModel* model, ca::StoredType storedType )
	{
		_notChangedTypes.clear();

		return evaluateChanges( model, storedType );

	}

	void update()
	{

	}

	bool SpaceUpdater::evaluateChanges( ca::IModel* model, ca::StoredType storedType )
	{
		co::IType* type = co::getType( storedType.typeName );

		co::TypeKind tk = type->getKind();

		if( tk < co::TK_ARRAY && tk != co::TK_ENUM && tk != co::TK_EXCEPTION ) //basic types never change
		{
			return false;
		}

		if( tk == co::TK_ARRAY )
		{
			co::IArray* arrayType = co::cast<co::IArray>( type );
			type = arrayType->getElementType();
		}

		if( _notChangedTypes.find( type ) != _notChangedTypes.end() )
		{
			return false;
		}

		std::vector<std::string> names;
		std::vector<co::IType*> types;

		if( tk == co::TK_COMPONENT )
		{
			std::vector<co::IPortRef> ports;
			model->getPorts( co::cast<co::IComponent>( type ), ports );

			for( int i = 0; i < ports.size(); i++ )
			{
				names.push_back( ports[i]->getName() );
				types.push_back( ports[i]->getType() );
			}

		}

		if( tk == co::TK_INTERFACE )
		{
			std::vector<co::IFieldRef> fields;
			model->getFields( co::cast<co::IInterface>( type ), fields );
			for( int i = 0; i < fields.size(); i++ )
			{
				names.push_back( fields[i]->getName() );
				types.push_back( fields[i]->getType() );
			}

		}

		if( names.size() != storedType.fields.size() )
		{
			return true;
		}

		ca::StoredType fieldStoredType;

		for( int i = 0; i < storedType.fields.size(); i++ )
		{

			if( storedType.fields[i].fieldName != names[i] )
			{
				return true;
			}

			_store->getType( storedType.fields[i].typeId, fieldStoredType );

			if( fieldStoredType.typeName != types[i]->getFullName() )
			{
				return true;
			}

			if ( fieldStoredType.typeName == storedType.typeName )
			{
				continue;
			}

			if( evaluateChanges( model, fieldStoredType ) )
			{
				return true;
			}
		}

		_notChangedTypes.insert( type );
		return false;
	}



} // namespace ca
#endif