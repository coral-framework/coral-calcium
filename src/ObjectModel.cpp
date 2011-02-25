/*
 * Calcium Object Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ObjectModel_Base.h"
#include <ca/ObjectModelException.h>
#include <co/Namespace.h>
#include <co/AttributeInfo.h>
#include <co/InterfaceInfo.h>
#include <co/InterfaceType.h>
#include <co/LifeCycleException.h>
#include <co/IllegalArgumentException.h>
#include <lua/IState.h>
#include <set>
#include <memory>
#include <vector>
#include <sstream>
#include <algorithm>

/*
	Anonymous namespace containing the low-level
	structures and functions used in object models.
 */
namespace
{
	/*
		Holds info about a type in the object model.
	 */
	struct TypeRecord
	{
		co::Type* type;

		// type kind
		co::TypeKind kind;

		// number of either attributes or receptacles
		co::int32 numMembers;

		// array of either attributes or receptacles
		union
		{
			co::AttributeInfo** attributes;
			co::InterfaceInfo** receptacles;
		};

		inline TypeRecord( co::Type* type ) : type( type ), attributes( NULL )
		{;}

		inline TypeRecord( co::Type* type, co::TypeKind kind )
			: type( type ), kind( kind ), numMembers( 0 ), attributes( NULL )
		{;}

		inline TypeRecord( co::Type* type, co::TypeKind kind, co::int32 numMembers )
			: type( type ), kind( kind ), numMembers( numMembers )
		{
			attributes = new co::AttributeInfo*[numMembers];
		}

		inline ~TypeRecord()
		{
			delete[] attributes;
		}
	};

	typedef std::vector<TypeRecord*> TypeList;

	inline bool typeRecordComparator( TypeRecord* a, TypeRecord* b )
	{
		return a->type < b->type;
	}

	TypeRecord* findType( TypeList& list, co::Type* type )
	{
		TypeRecord key( type );
		TypeList::iterator it = std::lower_bound( list.begin(), list.end(), &key, typeRecordComparator );
		if( it == list.end() )
			return NULL;
		TypeRecord* record = *it;
		return record->type == type ? record : NULL;
	}
}

namespace ca {

class ObjectModel : public ObjectModel_Base
{
public:
	ObjectModel()
	{
		_level = 0;
	}

	virtual ~ObjectModel()
	{
		// still have a pending transaction?
		if( _level > 0 )
		{
			_level = 1;
			discardChanges();
		}

		// delete all type records
		size_t count = _types.size();
		for( size_t i = 0; i < count; ++i )
			delete _types[i];
	}

	const std::string& getName()
	{
		return _name;
	}

	void setName( const std::string& name )
	{
		if( !_name.empty() )
			throw co::LifeCycleException( "once set, the name of an object model cannot be changed" );
		_name = name;
	}

	bool alreadyContains( co::Type* type )
	{
		assert( type );
		return findType( _types, type ) != NULL;
	}

	bool contains( co::Type* type )
	{
		assert( type );
		return getType( type ) != NULL;
	}

	co::ArrayRange<co::AttributeInfo* const> getAttributes( co::Type* type )
	{
		assert( type );
		TypeRecord* res = getTypeOrThrow( type );
		if( res->kind != co::TK_STRUCT && res->kind != co::TK_INTERFACE && res->kind != co::TK_NATIVECLASS )
			throw co::IllegalArgumentException( "type is not an attribute container" );
		return co::ArrayRange<co::AttributeInfo* const>( res->attributes, res->numMembers );
	}

	co::ArrayRange<co::InterfaceInfo* const> getReceptacles( co::ComponentType* componentType )
	{
		assert( componentType );
		TypeRecord* res = getTypeOrThrow( componentType );
		assert( res->kind == co::TK_COMPONENT );
		return co::ArrayRange<co::InterfaceInfo* const>( res->receptacles, res->numMembers );
	}

	void beginChanges()
	{
		assert( _level >= 0 );
		if( ++_level == 1 )
		{
			assert( _transactionTypes.empty() );
			assert( _transactionTypeSet.empty() );
			_discarded = false;
		}
	}

	void applyChanges()
	{
		assert( _level > 0 );

		if( _level < 1 )
			throw co::LifeCycleException( "no current transaction" );

		if( _discarded )
			throw co::LifeCycleException( "the current transaction was previously discarded" );

		if( _level == 1 )
		{
			validateTransaction();
			commitTransaction();
			assert( _level == 1 );
		}

		--_level;
	}

	void discardChanges()
	{
		assert( _level > 0 );
		_discarded = true;
		if( --_level == 0 )
			clearTransaction();
	}

	void addEnum( co::EnumType* type )
	{
		checkCanAddType( type );
		addToTransaction( new TypeRecord( type, co::TK_ENUM ) );
	}

	void addAttributeContainer( co::Type* type, co::ArrayRange<co::AttributeInfo* const> attributes )
	{
		checkCanAddType( type );

		co::TypeKind kind = type->getKind();
		if( kind != co::TK_STRUCT && kind != co::TK_INTERFACE && kind != co::TK_NATIVECLASS )
			CORAL_THROW( co::IllegalArgumentException, "type '" << type->getFullName() << "' is not an attribute container" );

		int size = static_cast<int>( attributes.getSize() );
		std::auto_ptr<TypeRecord> record( new TypeRecord( type, kind, size ) );

		co::InterfaceType* itf = dynamic_cast<co::InterfaceType*>( type );
		for( int i = 0; i < size; ++i )
		{
			co::AttributeInfo* ai = attributes[i];
			assert( !ai->getIsReadOnly() );
			co::CompoundType* ct = ai->getOwner();
			co::InterfaceType* ctItf = ( itf ? dynamic_cast<co::InterfaceType*>( ct ) : NULL );
			if( type == ct || ( ctItf && itf->isSubTypeOf( ctItf ) ) )
				record->attributes[i] = ai;
			else
				CORAL_THROW( co::IllegalArgumentException, "attribute '" << ai->getName() <<
								"' does not belong to type '" << type->getFullName() <<
								"', but to type '" << ct->getFullName() << "'" );
		}

		addToTransaction( record.release() );
	}

	void addComponent( co::ComponentType* componentType, co::ArrayRange<co::InterfaceInfo* const> receptacles )
	{
		checkCanAddType( componentType );

		if( componentType->getKind() != co::TK_COMPONENT )
			CORAL_THROW( co::IllegalArgumentException, "type '" << componentType->getFullName() << "' is not a component" )

		int size = static_cast<int>( receptacles.getSize() );
		std::auto_ptr<TypeRecord> record( new TypeRecord( componentType, co::TK_COMPONENT, size ) );

		for( int i = 0; i < size; ++i )
		{
			co::InterfaceInfo* ii = receptacles[i];
			assert( !ii->getIsFacet() );
			co::CompoundType* ct = ii->getOwner();
			if( componentType == ct )
				record->receptacles[i] = ii;
			else
				CORAL_THROW( co::IllegalArgumentException, "receptacle '" << ii->getName() <<
								"' does not belong to component '" << componentType->getFullName() <<
								"', but to component '" << ct->getFullName() << "'" );
		}

		addToTransaction( record.release() );
	}

protected:
	TypeRecord* getType( co::Type* type )
	{
		TypeRecord* res = findType( _types, type );

		/*
			If a type is not found and we haven't tried to load a
			CaModel file for it yet, do it and repeat the search.
		 */
		if( !res && loadCaModelFor( type ) )
			res = findType( _types, type );

		return res;
	}

	TypeRecord* getTypeOrThrow( co::Type* type )
	{
		TypeRecord* res = getType( type );		
		if( !res )
			CORAL_THROW( ca::ObjectModelException, "type '" << type->getFullName() << "' is not in the object model" );
		return res;
	}

	bool loadCaModelFor( co::Type* type )
	{
		if( _name.empty() )
			throw co::LifeCycleException( "the object model's name is required for this operation" );

		// have we tried to load a CaModel file from this type's namespace before?
		co::Namespace* ns = type->getNamespace();
		if( _visitedNamespaces.find( ns ) != _visitedNamespaces.end() )
			return false;

		_visitedNamespaces.insert( ns );

		// check if there's a matching CaModel file in the type's dir
		std::string filePath;
		std::string fileName;
		fileName.reserve( 64 );
		fileName += "CaModel_";
		fileName += _name;
		fileName += ".lua";
		if( !co::findModuleFile( ns->getFullName(), fileName, filePath ) )
			return false;

		co::Any args[2];
		args[0].set<ca::IObjectModel*>( this );
		args[1].set<const std::string&>( filePath );

		beginChanges();

		try
		{
			co::getService<lua::IState>()->callFunction( "ca.ModelLoader", std::string(),
											co::ArrayRange<const co::Any>( args, CORAL_ARRAY_LENGTH( args ) ),
											co::ArrayRange<const co::Any>() );
			applyChanges();
		}
		catch( co::Exception& e )
		{
			discardChanges();
			CORAL_THROW( ca::ObjectModelException, "error in CaModel file '" << filePath << "': " << e.getMessage() );
		}

		return true;
	}

	void checkCanAddType( co::Type* type )
	{
		if( _level < 1 )
			throw co::LifeCycleException( "no current transaction" );
		
		if( !type )
			throw co::IllegalArgumentException( "illegal null type" );

		if( alreadyContains( type ) )
			CORAL_THROW( ca::ObjectModelException, "type '" << type->getFullName()
							<< "' is already in the object model" );

		if( transactionContains( type ) )
			CORAL_THROW( ca::ObjectModelException, "type '" << type->getFullName()
							<< "' is already in the object model's transaction" );
	}

	void addToTransaction( TypeRecord* record )
	{
		_transactionTypes.push_back( record );
		_transactionTypeSet.insert( record->type );
	}

	bool transactionContains( co::Type* type )
	{
		return _transactionTypeSet.find( type ) != _transactionTypeSet.end();
	}

	void clearTransaction()
	{
		_transactionTypes.clear();
		TypeList emptyList;
		_transactionTypes.swap( emptyList );
		_transactionTypeSet.clear();
	}

	void validateTransaction()
	{
		TypeRecord* curType;
		co::MemberInfo* curMember;
		try
		{
			for( size_t i = 0; i < _transactionTypes.size(); ++i )
			{
				curType = _transactionTypes[i];
				co::TypeKind kind = curType->kind;
				if( kind == co::TK_ENUM )
				{
					// no need to validate structs
				}
				else if( kind == co::TK_COMPONENT )
				{
					// validate a component
					co::ComponentType* ct = dynamic_cast<co::ComponentType*>( curType->type );
					assert( ct );
					for( int k = 0; k < curType->numMembers; ++k )
					{
						curMember = curType->receptacles[k];
						validateTransactionTypeDependency( curType->receptacles[k]->getType() );
					}
				}
				else
				{
					// validate what must be an attribute container
					assert( kind == co::TK_STRUCT || kind == co::TK_INTERFACE || kind == co::TK_COMPONENT );
					co::AttributeContainer* ac = dynamic_cast<co::AttributeContainer*>( curType->type );
					assert( ac );
					for( int k = 0; k < curType->numMembers; ++k )
					{
						curMember = curType->attributes[k];
						validateTransactionTypeDependency( curType->attributes[k]->getType() );
					}
				}
			}
		}
		catch( co::Exception& e )
		{
			assert( curType && curMember );
			CORAL_THROW( ca::ObjectModelException, "in member '" << curMember->getName()
							<< "' of type '" << curType->type->getFullName() << "': " << e.getMessage() );
		}
	}

	void validateTransactionTypeDependency( co::Type* dependency )
	{
		switch( dependency->getKind() )
		{
		case co::TK_NONE:
			assert( false );
			break;
		case co::TK_ANY:
			throw ca::ObjectModelException( "attributes of type 'any' are currently forbidden" );
		case co::TK_BOOLEAN:
		case co::TK_INT8:
		case co::TK_UINT8:
		case co::TK_INT16:
		case co::TK_UINT16:
		case co::TK_INT32:
		case co::TK_UINT32:
		case co::TK_INT64:
		case co::TK_UINT64:
		case co::TK_FLOAT:
		case co::TK_DOUBLE:
		case co::TK_STRING:
			// no need to validate these primitive types
			break;
		case co::TK_ARRAY:
			validateTransactionTypeDependency( static_cast<co::ArrayType*>( dependency )->getElementType() );
			break;
		case co::TK_EXCEPTION:
			throw ca::ObjectModelException( "exceptions cannot be used as attribute types" );
		case co::TK_ENUM:
		case co::TK_STRUCT:
		case co::TK_NATIVECLASS:
		case co::TK_INTERFACE:
		case co::TK_COMPONENT:
			if( !transactionContains( dependency ) )
				getTypeOrThrow( dependency );
			break;
		}
	}

	void commitTransaction()
	{
		std::sort( _transactionTypes.begin(), _transactionTypes.end(), typeRecordComparator );

		size_t numTypes = _types.size();
		size_t numTransactionTypes = _transactionTypes.size();
		_types.reserve( numTypes + numTransactionTypes );
		_types.insert( _types.end(), _transactionTypes.begin(), _transactionTypes.end() );

		std::inplace_merge( _types.begin(), _types.begin() + numTypes, _types.end(), typeRecordComparator );

		clearTransaction();
	}

private:
	// --- regular fields --- //

	std::string _name;

	// sorted list of types in the object model
	TypeList _types;

	// namespaces we tried to load a CaModel file from (avoids retries)
	std::set<co::Namespace*> _visitedNamespaces;

	// --- transaction fields --- //

	int _level;
	bool _discarded;

	// non-sorted list of types in the transaction
	TypeList _transactionTypes;

	// helper set for checking whether a type is in the transaction
	std::set<co::Type*> _transactionTypeSet;
};

} // namespace ca

CORAL_EXPORT_COMPONENT( ca::ObjectModel, ObjectModel )
