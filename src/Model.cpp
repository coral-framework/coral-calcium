/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Model.h"
#include <ca/ModelException.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>
#include <lua/IState.h>
#include <sstream>

namespace ca {

void TypeRecord::destroy()
{
	if( kind == TRK_RECORD )
	{
		RecordTypeRecord* rec = static_cast<RecordTypeRecord*>( this );
		for( co::uint16 i = 0; i < rec->numFields; ++i )
		{
			co::IReflector* reflector = rec->fields[i].reflector;
			if( reflector )
				reflector->serviceRelease();
		}
	}

	free( this );
}

void RecordTypeRecord::addField( FieldKind fieldKind, co::IField* field, co::IReflector* reflector )
{
	co::uint16 idx;
	if( fieldKind == FK_Value )
	{
		// add to the last free position
		++numValues;
		idx = --firstValue;
	}
	else if( fieldKind == FK_RefVec )
	{
		idx = numRefs + numRefVecs++;
	}
	else
	{
		// we may have to shift RefVecs to the right, to free one Ref position
		for( co::uint16 i = numRefs + numRefVecs; i > numRefs; --i )
			fields[i] = fields[i - 1];

		idx = numRefs++;
	}

	assert( numRefs + numRefVecs <= firstValue );
	assert( firstValue + numValues == numFields );

	fields[idx].field = field;
	fields[idx].reflector = reflector;

	reflector->serviceRetain();
}

void ComponentRecord::addPort( co::IPort* port )
{
	// group receptacles at the start and facets at the end
	int idx = port->getIsFacet() ? ( numPorts - ++numFacets ) : numReceptacles++;
	ports[idx].port = port;
}

void ComponentRecord::computeLayout()
{
	assert( numReceptacles + numFacets == numPorts );
	assert( numRefs == 0 && numRefVecs == 0 && numValues == 0 );

	// process receptacles
	numRefs = numReceptacles;
	for( co::uint8 i = 0; i < numReceptacles; ++i )
		ports[i].firstRef = i;

	// process facets
	for( co::uint8 i = numReceptacles; i < numPorts; ++i )
	{
		PortRecord& facet = ports[i];
		assert( facet.typeRec );

		facet.firstRef = numRefs;
		numRefs += facet.typeRec->numRefs;

		facet.firstRefVec = numRefVecs;
		numRefVecs += facet.typeRec->numRefVecs;

		facet.firstValue = numValues;
		numValues += facet.typeRec->numValues;
	}

	// compute the memory layout/size
	co::uint32 size = sizeof(ObjectRecord);
	assert( size % alignmentOf<RefField>::value == 0 );

	CORAL_STATIC_CHECK( sizeof(RefField) == sizeof(RefVecField), optimization_assumption_failed );
	size += sizeof(RefField) * ( numRefs + numRefVecs );

	// guarantee proper alignment for ValueFields
	offset1stValue = align<alignmentOf<ValueField>::value>( size );
	assert( offset1stValue >= size );

	objectSize = offset1stValue + sizeof(ValueField) * numValues;
}

ObjectRecord* ObjectRecord::create( ComponentRecord* model, co::IObject* instance )
{
	ObjectRecord* rec = reinterpret_cast<ObjectRecord*>( calloc( 1, model->objectSize ) );
	rec->model = model;
	rec->instance = instance;

	new( &rec->spaceRefs ) SpaceRefCountMap();

	instance->serviceRetain();

	return rec;
}

void ObjectRecord::destroy()
{
	// object should have no dangling references to it
	assert( inDegree == 0 );

	// object should contain no references to other objects
	assert( outDegree == 0 );

	// destroy the std::map
	spaceRefs.~map();

	// destroy all RefVecFields
	RefVecField* refVecs = getRefVec( 0 );
	for( co::uint16 i = 0; i < model->numRefVecs; ++i )
		free( refVecs[i].services );

	// destroy all ValueFields
	ValueField* values = getValue( 0 );
	for( co::uint16 i = 0; i < model->numValues; ++i )
		values[i].~Any();

	instance->serviceRelease();

	free( this );
}

Model::Model()
{
	_level = 0;
}

Model::~Model()
{
	// still have a pending transaction?
	if( _level > 0 )
	{
		_level = 1;
		discardChanges();
	}
	assert( _transaction.empty() );

	// delete all type records
	size_t count = _types.size();
	for( size_t i = 0; i < count; ++i )
		_types[i]->destroy();
}

RecordTypeRecord* Model::getRecordType( co::IRecordType* recordType )
{
	assert( recordType );
	TypeRecord* rec = getTypeOrThrow( recordType );
	assert( rec->isRecordType() );
	return static_cast<RecordTypeRecord*>( rec );
}

ComponentRecord* Model::getComponent( co::IComponent* component )
{
	assert( component );
	TypeRecord* rec = getTypeOrThrow( component );
	assert( rec->isComponent() );
	return static_cast<ComponentRecord*>( rec );
}

const std::string& Model::getName()
{
	return _name;
}

void Model::setName( const std::string& name )
{
	if( !_name.empty() )
		throw co::IllegalStateException( "once set, the name of a ca.Model cannot be changed" );
	_name = name;
}

bool Model::alreadyContains( co::IType* type )
{
	assert( type );
	return findType( _types, type ) != NULL;
}

bool Model::contains( co::IType* type )
{
	assert( type );
	return getType( type ) != NULL;
}

void Model::getFields( co::IRecordType* recordType, co::RefVector<co::IField>& fields )
{
	RecordTypeRecord* rec = getRecordType( recordType );
	fields.clear();
	fields.reserve( rec->numFields );
	for( co::int32 i = 0; i < rec->numFields; ++i )
		fields.push_back( rec->fields[i].field );
}

void Model::getPorts( co::IComponent* component, co::RefVector<co::IPort>& ports )
{
	ComponentRecord* rec = getComponent( component );
	ports.clear();
	ports.reserve( rec->numPorts );
	for( co::uint8 i = 0; i < rec->numPorts; ++i )
		ports.push_back( rec->ports[i].port );
}

void Model::beginChanges()
{
	assert( _level >= 0 );
	if( ++_level == 1 )
	{
		assert( _transaction.empty() );
		_discarded = false;
	}
}

void Model::applyChanges()
{
	assert( _level > 0 );

	if( _level < 1 )
		throw co::IllegalStateException( "no current transaction" );

	if( _discarded )
		throw co::IllegalStateException( "the current transaction was previously discarded" );

	if( _level == 1 )
	{
		validateTransaction();
		commitTransaction();
		assert( _level == 1 );
	}

	--_level;
}

void Model::discardChanges()
{
	assert( _level > 0 );
	_discarded = true;
	if( --_level == 0 )
	{
		for( TypeSet::iterator it = _transaction.begin(); it != _transaction.end(); ++it )
			( *it )->destroy();
		_transaction.clear();
	}
}

void Model::addEnum( co::IEnum* enumType )
{
	checkCanAddType( enumType );
	_transaction.insert( TypeRecord::create( enumType ) );
}

void Model::addRecordType( co::IRecordType* recordType, co::Range<co::IField* const> fields )
{
	checkCanAddType( recordType );

	co::uint16 numFields = static_cast<co::uint16>( fields.getSize() );
	RecordTypeRecord* rec = RecordTypeRecord::create( recordType, numFields );

	try
	{
		co::IInterface* itf = ( recordType->getKind() == co::TK_INTERFACE ?
						   static_cast<co::IInterface*>( recordType ) : NULL );

		for( co::int32 i = 0; i < numFields; ++i )
		{
			co::IField* field = fields[i];
			assert( !field->getIsReadOnly() );

			FieldKind fieldKind = fieldKindOf( field->getType() );

			co::ICompositeType* ct = field->getOwner();
			bool fieldIsValid;
			if( itf )
			{
				// interfaces accept all field kinds and support inheritance
				co::IInterface* ctItf = ( ct->getKind() == co::TK_INTERFACE ?
									static_cast<co::IInterface*>( ct ) : NULL );
				fieldIsValid = ( ctItf && itf->isSubTypeOf( ctItf ) );
			}
			else
			{
				// complex values can only contain value-typed fields
				if( fieldKind != FK_Value )
					CORAL_THROW( co::IllegalArgumentException, "illegal reference field '"
						<< field->getName() << "' in complex value type '" << ct->getFullName() << "'" );

				fieldIsValid = ( recordType == ct );
			}

			if( fieldIsValid )
				rec->addField( fieldKind, field, ct->getReflector() );
			else
				CORAL_THROW( co::IllegalArgumentException, "field '" << field->getName() <<
								"' does not belong to type '" << recordType->getFullName() <<
								"', but to type '" << ct->getFullName() << "'" );
		}
	}
	catch( ... )
	{
		rec->destroy();
		throw;
	}

	_transaction.insert( rec );
}

void Model::addComponent( co::IComponent* component, co::Range<co::IPort* const> ports )
{
	checkCanAddType( component );

	co::uint8 numPorts = static_cast<co::uint8>( ports.getSize() );
	ComponentRecord* rec = ComponentRecord::create( component, numPorts );

	try
	{
		for( co::uint8 i = 0; i < numPorts; ++i )
		{
			co::IPort* port = ports[i];
			co::ICompositeType* ct = port->getOwner();
			if( component != ct )
				CORAL_THROW( co::IllegalArgumentException, "port '" << port->getName() <<
					"' does not belong to component '" << component->getFullName() <<
					"', but to component '" << ct->getFullName() << "'" );

			rec->addPort( port );
		}
	}
	catch( ... )
	{
		rec->destroy();
		throw;
	}

	_transaction.insert( rec );
}

TypeRecord* Model::getType( co::IType* type )
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

TypeRecord* Model::getTypeOrThrow( co::IType* type )
{
	TypeRecord* res = getType( type );		
	if( !res )
		CORAL_THROW( ca::ModelException, "type '" << type->getFullName() << "' is not in the object model" );
	return res;
}

bool Model::loadCaModelFor( co::IType* type )
{
	if( _name.empty() )
		throw co::IllegalStateException( "the ca.Model's name is required for this operation" );

	// have we tried to load a CaModel file from this type's namespace before?
	co::INamespace* ns = type->getNamespace();
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
	args[0].set<ca::IModel*>( this );
	args[1].set<const std::string&>( filePath );

	beginChanges();

	try
	{
		co::getService<lua::IState>()->callFunction( "ca.ModelLoader", std::string(),
						co::Range<const co::Any>( args, CORAL_ARRAY_LENGTH( args ) ),
						co::Range<const co::Any>() );
		applyChanges();
	}
	catch( co::Exception& e )
	{
		discardChanges();
		CORAL_THROW( ca::ModelException, "error in CaModel file '" << filePath
						<< "': " << e.getMessage() );
	}

	return true;
}

void Model::checkCanAddType( co::IType* type )
{
	if( _level < 1 )
		throw co::IllegalStateException( "no current transaction" );

	if( !type )
		throw co::IllegalArgumentException( "illegal null type" );

	if( alreadyContains( type ) )
		CORAL_THROW( ca::ModelException, "type '" << type->getFullName()
						<< "' is already in the object model" );

	if( findTransactionType( type ) )
		CORAL_THROW( ca::ModelException, "type '" << type->getFullName()
						<< "' is already in the model's transaction" );
}

void Model::validateTransaction()
{
	TypeRecord* typeRec;
	co::IMember* member;
	try
	{
		for( TypeSet::iterator it = _transaction.begin(); it != _transaction.end(); ++it )
		{
			typeRec = *it;
			if( typeRec->isEnum() )
			{
				// no need to validate enums
			}
			else if( typeRec->isComponent() )
			{
				// validate a component
				ComponentRecord* rec = static_cast<ComponentRecord*>( typeRec );
				for( co::uint8 k = 0; k < rec->numPorts; ++k )
				{
					co::IPort* port = rec->ports[k].port;
					member = port;
					TypeRecord* depRec = validateTypeDependency( port->getType() );
					assert( depRec->isRecordType() );
					rec->ports[k].typeRec = static_cast<RecordTypeRecord*>( depRec );
				}
				rec->computeLayout();
			}
			else
			{
				// validate what must be a record type
				assert( typeRec->isRecordType() );
				RecordTypeRecord* rec = static_cast<RecordTypeRecord*>( typeRec );
				for( co::uint16 k = 0; k < rec->numFields; ++k )
				{
					co::IField* field = rec->fields[k].field;
					member = field;
					validateTypeDependency( field->getType() );
				}
			}
		}
	}
	catch( co::Exception& e )
	{
		CORAL_THROW( ca::ModelException, "in member '" << member->getName()
			<< "' of type '" << typeRec->type->getFullName() << "': " << e.getMessage() );
	}
}

TypeRecord* Model::validateTypeDependency( co::IType* dependency )
{
	TypeRecord* res = NULL;
	switch( dependency->getKind() )
	{
	case co::TK_NONE:
		assert( false );
		break;
	case co::TK_ANY:
		throw ca::ModelException( "fields of type 'any' are currently forbidden" );
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
		res = validateTypeDependency( static_cast<co::IArray*>( dependency )->getElementType() );
		break;
	case co::TK_EXCEPTION:
		throw ca::ModelException( "exceptions cannot be used as field types" );
	case co::TK_ENUM:
	case co::TK_STRUCT:
	case co::TK_NATIVECLASS:
	case co::TK_INTERFACE:
	case co::TK_COMPONENT:
		res = findTransactionType( dependency );
		if( !res )
			res = getTypeOrThrow( dependency );
		break;
	}

	return res;
}

void Model::commitTransaction()
{
	size_t numTypes = _types.size();
	size_t numTransactionTypes = _transaction.size();
	_types.reserve( numTypes + numTransactionTypes );
	_types.insert( _types.end(), _transaction.begin(), _transaction.end() );
	std::inplace_merge( _types.begin(), _types.begin() + numTypes, _types.end(), TypeRecordComparator() );
	_transaction.clear();
}

CORAL_EXPORT_COMPONENT( Model, Model )

} // namespace ca
