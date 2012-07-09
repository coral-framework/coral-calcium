/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "Model.h"
#include <ca/ModelException.h>
#include <co/IllegalStateException.h>
#include <co/IllegalArgumentException.h>
#include <lua/IState.h>
#include <co/ITypeManager.h>
#include <co/ISystem.h>
#include <algorithm>
#include <sstream>

namespace ca {

/*
	Assuming Struct and ElementType are POD types, allocates memory for Struct
	while making room for a number of extra instances of ElementType at the end.
	All bits are initialized to zero. Memory must be deallocated using free().
 */
template<typename Struct, typename ElementType>
inline Struct* allocateExpanded( co::uint32 numElems )
{
	size_t size = sizeof(Struct) + ( !numElems ? 0 : ( numElems - 1 ) * sizeof(ElementType) );
	return reinterpret_cast<Struct*>( calloc( 1, size ) );
}

TypeRecord* TypeRecord::create( co::IEnum* type )
{
	TypeRecord* rec = reinterpret_cast<TypeRecord*>( malloc( sizeof( TypeRecord ) ) );
	rec->init( type, TRK_ENUM );
	return rec;
}

RecordRecord* RecordRecord::create( co::IRecordType* type, co::uint16 numFields )
{
	assert( type->getKind() != co::TK_INTERFACE );
	RecordRecord* rec = allocateExpanded<RecordRecord, co::IField*>( numFields );
	rec->init( type, TRK_RECORD );
	rec->numFields = 0;
	return rec;
}

bool compareIFieldNames( co::IField* a, co::IField* b )
{
	return a->getName() < b->getName();
}

void RecordRecord::finalize()
{
	std::sort( fields, fields + numFields, compareIFieldNames );
}

InterfaceRecord* InterfaceRecord::create( co::IInterface* type, co::uint16 numFields )
{
	InterfaceRecord* rec = allocateExpanded<InterfaceRecord, FieldRecord>( numFields );
	rec->init( type, TRK_INTERFACE );
	rec->firstValue = numFields;
	rec->numFields = numFields;
	return rec;
}

void InterfaceRecord::addField( FieldKind fieldKind, co::IField* field )
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

	fields[idx].field = field;
}

// Utility function to help compute aligned offsets.
inline co::uint32 align( co::uint32 offset, co::uint32 alignment )
{
	return ( offset + alignment - 1 ) & ~( alignment - 1 );
}

inline bool compareFieldNames( const FieldRecord& a, const FieldRecord& b )
{
	return a.field->getName() < b.field->getName();
}

inline bool compareFieldSizes( const FieldRecord& a, const FieldRecord& b )
{
	return a.getSize() > b.getSize();
}

void InterfaceRecord::finalize()
{
	if( size ) return; // already computed

	assert( numRefs + numRefVecs == firstValue );
	assert( numRefs + numRefVecs + numValues == numFields );

	// sort Refs and RefVecs by name
	std::sort( &fields[0], &fields[numRefs], compareFieldNames );
	std::sort( &fields[numRefs], &fields[firstValue], compareFieldNames );

	// sort values by descending size
	std::sort( &fields[firstValue], &fields[numFields], compareFieldSizes );

	// allocate all fields:
	co::uint16 i = 0;
	co::uint32 offset = 0;

	// first all Ref and RefVec fields
	static_assert( sizeof(RefField) == sizeof(RefVecField), "unexpected size mismatch" );
	for( ; i < firstValue; ++i )
	{
		fields[i].offset = offset;
		offset += sizeof(RefField);
	}

	// then all Value fields
	for( ; i < numFields; ++i )
	{
		// guarantee proper alignment
		co::uint32 size = fields[i].getSize();
		assert( size > 0 && ( size >= sizeof(double) || size == 4 || size < 3 ) );
		offset = align( offset, ( size >= sizeof(double) ? sizeof(double) : size ) );

		fields[i].offset = offset;
		offset += size;
	}

	size = offset;

	// re-sort the values by name
	std::sort( &fields[firstValue], &fields[numFields], compareFieldNames );
}

ComponentRecord* ComponentRecord::create( co::IComponent* type, co::uint8 numPorts )
{
	ComponentRecord* rec = allocateExpanded<ComponentRecord, PortRecord>( numPorts );
	rec->init( type, TRK_COMPONENT );
	rec->numPorts = numPorts;
	return rec;
}

void ComponentRecord::addPort( co::IPort* port )
{
	// group facets at the start and receptacles at the end
	int idx = port->getIsFacet() ? numFacets++ : ( numPorts - ++numReceptacles );
	ports[idx].port = port;
}

inline bool comparePortNames( const PortRecord& a, const PortRecord& b )
{
	return a.port->getName() < b.port->getName();
}

void ComponentRecord::finalize()
{
	assert( objectSize == 0 );
	assert( numReceptacles + numFacets == numPorts );

	// sort ports by name
	std::sort( &ports[0], &ports[numFacets], comparePortNames );
	std::sort( &ports[numFacets], &ports[numPorts], comparePortNames );

	// we start off at &services[0] within the ObjectRecord
	co::uint32 offset = ( sizeof(ObjectRecord) - sizeof(void*) );

	// allocate the facet refs
	offset += sizeof(void*) * numFacets;

	// allocate all receptacles
	for( co::uint8 i = numFacets; i < numPorts; ++i )
	{
		ports[i].offset = offset;
		offset += sizeof(RefField);
	}

	// allocate all facet data blocks
	for( co::uint8 i = 0; i < numFacets; ++i )
	{
		/*
			Guarantee pointer alignment at the beginning of each data block,
			since we always start off with an array of RefFields.
		 */
		offset = align( offset, sizeof(void*) );
		ports[i].offset = offset;
		ports[i].typeRec->finalize();
		offset += ports[i].typeRec->size;
	}

	objectSize = offset;
}

struct ObjectCreationTraverser : public Traverser<ObjectCreationTraverser>
{
	ObjectCreationTraverser( ObjectRecord* object ) : T( object )
	{;}

	void onReceptacle( PortRecord& receptacle, RefField& ref )
	{
		ref.service = NULL;
		ref.object = NULL;
	}

	void onRefField( co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		ref.service = NULL;
		ref.object = NULL;
	}

	void onRefVecField( co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		refVec.services = NULL;
		refVec.objects = NULL;
	}

	void onValueField( co::uint8 facetId, FieldRecord& field, void* valuePtr )
	{
		field.getTypeReflector()->createValues( valuePtr, 1 );
	}
};

ObjectRecord* ObjectRecord::create( ComponentRecord* model, co::IObject* instance )
{
	ObjectRecord* rec = reinterpret_cast<ObjectRecord*>( malloc( model->objectSize ) );
	rec->model = model;
	rec->instance = instance;

	rec->inDegree = 0;
	rec->outDegree = 0;

	new( &rec->spaceRefs ) SpaceRefCountMap();

	// initialize the facet refs
	for( co::uint8 i = 0; i < model->numFacets; ++i )
		rec->services[i] = instance->getServiceAt( model->ports[i].port );

	// initialize all fields
	ObjectCreationTraverser traverser( rec );
	traverser.traverseObject();

	instance->serviceRetain();

	return rec;
}

const char* getServiceTypeName( ObjectRecord* object, co::int16 facetId )
{
	return facetId < 0 ?
		"co.IObject" :
		object->model->ports[facetId].typeRec->type->getFullName().c_str();
}

struct ObjectDestructionTraverser : public Traverser<ObjectDestructionTraverser>
{
	ObjectDestructionTraverser( ObjectRecord* object ) : T( object )
	{;}

	void onReceptacle( PortRecord& receptacle, RefField& ref )
	{
		// NOP
	}

	void onRefField( co::uint8 facetId, FieldRecord& field, RefField& ref )
	{
		// NOP
	}

	void onRefVecField( co::uint8 facetId, FieldRecord& field, RefVecField& refVec )
	{
		refVec.destroy();
	}

	void onValueField( co::uint8 facetId, FieldRecord& field, void* valuePtr )
	{
		field.getTypeReflector()->destroyValues( valuePtr, 1 );
	}
};

void ObjectRecord::destroy()
{
	// object should have no dangling references to it
	assert( inDegree == 0 );

	// object should contain no references to other objects
	assert( outDegree == 0 );

	// destroy the std::map
	spaceRefs.~map();

	// destroy all fields
	ObjectDestructionTraverser traverser( this );
	traverser.traverseObject();

	instance->serviceRelease();

	free( this );
}

/******************************************************************************/
/* ca.Model                                                                   */
/******************************************************************************/

Model::ComponentList Model::sm_components;

bool Model::contains( co::IComponent* ct )
{
	ComponentList::iterator it = std::lower_bound( sm_components.begin(), sm_components.end(), ct );
	return it != sm_components.end() && *it == ct;
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
	size_t numTypes = _types.size();
	for( size_t i = 0; i < numTypes; ++i )
		_types[i]->destroy();
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

co::Range<std::string const> Model::getUpdates()
{
	return _updates;
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
	TypeRecord* typeRec = getTypeOrThrow( recordType );
	fields.clear();
	if( typeRec->kind == TRK_INTERFACE )
	{
		InterfaceRecord* rec = static_cast<InterfaceRecord*>( typeRec );
		fields.reserve( rec->numFields );
		for( co::int32 i = 0; i < rec->numFields; ++i )
			fields.push_back( rec->fields[i].field );
	}
	else
	{
		RecordRecord* rec = static_cast<RecordRecord*>( typeRec );
		fields.reserve( rec->numFields );
		for( co::int32 i = 0; i < rec->numFields; ++i )
			fields.push_back( rec->fields[i] );
	}
}

void Model::getPorts( co::IComponent* component, co::RefVector<co::IPort>& ports )
{
	ComponentRecord* rec = getComponentRec( component );
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
		size_t numTransactionTypes = _transaction.size();
		for( size_t i = 0; i < numTransactionTypes; ++i )
			_transaction[i]->destroy();

		_transaction.clear();
	}
}

void Model::addEnum( co::IEnum* enumType )
{
	checkCanAddType( enumType );
	_transaction.push_back( TypeRecord::create( enumType ) );
}

void Model::addRecordType( co::IRecordType* recordType, co::Range<co::IField* const> fields )
{
	checkCanAddType( recordType );

	co::uint16 numFields = static_cast<co::uint16>( fields.getSize() );

	co::IInterface* itf;
	union
	{
		TypeRecord* rec;
		RecordRecord* recRec;
		InterfaceRecord* itfRec;
	};

	if( recordType->getKind() == co::TK_INTERFACE )
	{
		itf = static_cast<co::IInterface*>( recordType );
		itfRec = InterfaceRecord::create( itf, numFields );
	}
	else
	{
		itf = NULL;
		recRec = RecordRecord::create( recordType, numFields );
	}

	try
	{
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
				if( itf )
					itfRec->addField( fieldKind, field );
				else
					recRec->addField( field );
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

	_transaction.push_back( rec );
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

	_transaction.push_back( rec );
}

void Model::addUpdate( const std::string& update )
{
	_updates.push_back( update );
}

bool Model::loadDefinitionsFor( const std::string& moduleName )
{
	co::INamespace* ns = co::getSystem()->getTypes()->getNamespace( moduleName );
	return ns == NULL ? false : loadDefinitionsFor( ns );
}

TypeRecord* Model::getType( co::IType* type )
{
	TypeRecord* res = findType( _types, type );

	/*
		If a type is not found and we haven't tried to load a
		CaModel file for it yet, do it and repeat the search.
	 */
	if( !res && loadDefinitionsFor( type ) )
	{
		if( _level >= 1 )
			res = findTransactionType( type );
		else
			res = findType( _types, type );
	}

	return res;
}

TypeRecord* Model::getTypeOrThrow( co::IType* type )
{
	TypeRecord* res = getType( type );		
	if( !res )
		CORAL_THROW( ca::ModelException, "type '" << type->getFullName() << "' is not in the object model" );
	return res;
}

bool Model::loadDefinitionsFor( co::INamespace* ns )
{
	if( _name.empty() )
		throw co::IllegalStateException( "the ca.Model's name is required for this operation" );

	if( _visitedNamespaces.find( ns ) != _visitedNamespaces.end() )
		return false;

	_visitedNamespaces.insert( ns );

	std::string filePath;
	std::string fileName;
	fileName.reserve( 64 );
	fileName += "CaModel_";
	fileName += _name;
	fileName += ".lua";
	if( !co::findFile( ns->getFullName(), fileName, filePath ) )
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

bool Model::loadDefinitionsFor( co::IType* type )
{
	assert( type );
	return loadDefinitionsFor( type->getNamespace() );
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
		for( size_t i = 0; i < _transaction.size(); ++i )
		{
			typeRec = _transaction[i];
			switch( typeRec->kind )
			{
			case TRK_ENUM: break; // no need to validate enums
			case TRK_RECORD:
				{
					RecordRecord* rec = static_cast<RecordRecord*>( typeRec );
					for( co::uint16 k = 0; k < rec->numFields; ++k )
					{
						co::IField* field = rec->fields[k];
						member = field;
						validateTypeDependency( field->getType() );
					}
					rec->finalize();
				}
				break;
			case TRK_INTERFACE:
				{
					InterfaceRecord* rec = static_cast<InterfaceRecord*>( typeRec );
					for( co::uint16 k = 0; k < rec->numFields; ++k )
					{
						co::IField* field = rec->fields[k].field;
						member = field;
						validateTypeDependency( field->getType() );
					}
					rec->finalize();
				}
				break;
			case TRK_COMPONENT:
				{
					ComponentRecord* rec = static_cast<ComponentRecord*>( typeRec );
					for( co::uint8 k = 0; k < rec->numPorts; ++k )
					{
						co::IPort* port = rec->ports[k].port;
						member = port;
						TypeRecord* depRec = validateTypeDependency( port->getType() );
						assert( depRec->isInterface() );
						rec->ports[k].typeRec = static_cast<InterfaceRecord*>( depRec );
					}
					rec->finalize();
				}
				break;
			default:
				assert( false );
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

	std::sort( _transaction.begin(), _transaction.end(), TypeRecordComparator() );

	size_t numAddedComponents = 0;
	for( size_t i = 0; i < numTransactionTypes; ++i )
	{
		TypeRecord* rec = _transaction[i];
		_types.push_back( rec );
		if( rec->isComponent() )
		{
			co::IComponent* ct = static_cast<co::IComponent*>( rec->type );
			if( !Model::contains( ct ) )
			{
				sm_components.push_back( ct );
				++numAddedComponents;
			}
		}
	}

	_transaction.clear();

	std::inplace_merge( _types.begin(), _types.begin() + numTypes, _types.end(), TypeRecordComparator() );

	if( numAddedComponents )
	{
		std::inplace_merge( sm_components.begin(),
			sm_components.begin() + sm_components.size() - numAddedComponents,
			sm_components.end() );
	}
}

CORAL_EXPORT_COMPONENT( Model, Model )

} // namespace ca
