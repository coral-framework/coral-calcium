/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#ifndef _CA_MODEL_H_
#define _CA_MODEL_H_

#include "Model_Base.h"
#include <co/IEnum.h>
#include <co/IPort.h>
#include <co/IField.h>
#include <co/IComponent.h>
#include <co/IInterface.h>
#include <co/INamespace.h>
#include <co/IReflector.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>

namespace ca {

/*
	Assuming IStruct and ElementType are POD types, allocates memory for IStruct
	while making room for a number of extra instances of ElementType at the end.
	All bits are initialized to zero. Memory must be deallocated using free().
 */
template<typename IStruct, typename ElementType>
inline IStruct* allocateExpanded( co::uint32 numElems )
{
	size_t size = sizeof(IStruct) + ( !numElems ? 0 : ( numElems - 1 ) * sizeof(ElementType) );
	return reinterpret_cast<IStruct*>( calloc( 1, size ) );
}

/*
	Base record for a type in the model. Only directly used for enums.
 */
struct TypeRecord
{
	co::IType* type;

	enum TypeRecordKind { TRK_ENUM, TRK_RECORD, TRK_COMPONENT };
	TypeRecordKind kind;

	inline TypeRecord( co::IType* type ) : type( type )
	{;}

	inline bool isEnum() const { return kind == TRK_ENUM; }

	inline bool isRecordType() const { return kind == TRK_RECORD; }

	inline bool isComponent() const { return kind == TRK_COMPONENT; }

	void destroy();

	static TypeRecord* create( co::IEnum* type )
	{
		TypeRecord* rec = reinterpret_cast<TypeRecord*>( malloc( sizeof( TypeRecord ) ) );
		rec->type = type;
		rec->kind = TRK_ENUM;
		return rec;
	}
};

// STL comparator class for keeping TypeRecords in sorted containers
struct TypeRecordComparator
{
	inline bool operator()( TypeRecord* a, TypeRecord* b ) const
	{
		return a->type < b->type;
	}
};

typedef std::set<TypeRecord*, TypeRecordComparator> TypeSet;
typedef std::vector<TypeRecord*> TypeList;

// Helper function to locate a type's record in a sorted vector using binary search.
inline TypeRecord* findType( TypeList& list, co::IType* type )
{
	TypeRecord key( type );
	TypeList::iterator it = std::lower_bound( list.begin(), list.end(), &key, TypeRecordComparator() );
	if( it == list.end() )
		return NULL;
	TypeRecord* record = *it;
	return record->type == type ? record : NULL;
}

/*
	Supported kinds of fields in an object.
 */
enum FieldKind
{
	FK_Ref,		// a reference to a service
	FK_RefVec,	// an array of references to services
	FK_Value	// an acceptable Coral value (which must not contain a reference)
};

/*
	Returns the FieldKind of a type.
 */
inline FieldKind fieldKindOf( co::IType* type )
{
	co::TypeKind kind = type->getKind();
	FieldKind res = FK_Value;
	if( kind == co::TK_ARRAY )
	{
		if( static_cast<co::IArray*>( type )->getElementType()->getKind() == co::TK_INTERFACE )
			res = FK_RefVec;
	}
	else if( kind == co::TK_INTERFACE )
		res = FK_Ref;
	return res;
}

/*
	Represents a field within a RecordTypeRecord.
 */
struct FieldRecord
{
	co::IField* field;
	co::IReflector* reflector;
};

/*
	Record for a RecordType (interface, struct or native class) in the model.

	Field records (in the 'fields' array) are sorted/grouped by FieldKind:
		- The first 'numRefs' fields in the array are FK_Ref's.
		- The following 'numRefVecs' fields are FK_RefVec's.
		- The final 'numValues' fields are FK_Value's.
 */
struct RecordTypeRecord : TypeRecord
{
	co::uint8 numRefs;		// references are in fields[0..numRefs-1]
	co::uint8 numRefVecs;	// ref-vecs are in fields[numRefs..firstValue-1]
	co::uint8 numValues;	// values are in fields[firstValue..numFields-1]

	co::uint16 firstValue;		// numRefs + numRefVecs
	co::uint16 numFields;	// numRefs + numRefVecs + numValues

	FieldRecord fields[1];

	void addField( FieldKind fieldKind, co::IField* field, co::IReflector* reflector );

	static RecordTypeRecord* create( co::IRecordType* recordType, co::uint16 numFields )
	{
		RecordTypeRecord* rec = allocateExpanded<RecordTypeRecord, FieldRecord>( numFields );
		rec->type = recordType;
		rec->kind = TRK_RECORD;
		rec->firstValue = numFields;
		rec->numFields = numFields;
		return rec;
	}
};

/*
	Represents a port within a ComponentRecord.
 */
struct PortRecord
{
	co::IPort* port;			// identifies the port within the component
	RecordTypeRecord* typeRec;	// only relevant for facets

	/*
		For facets:
			- firstRef = position of the facet's first Ref field in the object's Ref block.
			- firstRefVec = position of the facet's first RefVec field in the object's RefVec block.
			- firstValue = position of the facet's first Value field in the object's Value block.
		For receptacles: only firstRef is used (other values are always zero).
	 */
	co::uint16 firstRef;
	co::uint16 firstRefVec;
	co::uint16 firstValue;
};

/*
	ComponentRecords are models for calcium objects.

	The set of receptacles and facets defined for a component determines
	the appropriate memory layout for the objects.
 */
struct ComponentRecord : TypeRecord
{
	co::uint8 numReceptacles;	// receptacles are in ports[0..numReceptacles-1]
	co::uint8 numFacets;		// facets are in ports[numReceptacles..numPorts-1]
	co::uint8 numPorts;			// numReceptacles + numFacets

	/*
		The memory layout of an object is as follows:
			[ObjectRecord data] // Meta-object data; see the ObjectRecord struct.
			[References]		// Single references, for all receptacles and facets.
			[RefVectors]		// Reference vectors for all facets.
			[Values]			// Values for all facets.
	 */
	co::uint16 numRefs;			// total number of Refs in the component
	co::uint16 numRefVecs;		// total number of RefVecs in the component
	co::uint16 numValues;		// total number of Values in the component

	co::uint32 objectSize;		// total number of bytes needed for an object instance
	co::uint32 offset1stValue;	// offset from the start of the ObjectRecord to the first value

	// Receptacles are grouped at the start, facets at the end
	PortRecord ports[1];

	// Adds a port, keeping the array correctly sorted.
	void addPort( co::IPort* port );

	// Must be called only after all ports have been initialized with their typeRec.
	void computeLayout();

	static ComponentRecord* create( co::IComponent* component, co::uint8 numPorts )
	{
		ComponentRecord* rec = allocateExpanded<ComponentRecord, PortRecord>( numPorts );
		rec->type = component;
		rec->kind = TRK_COMPONENT;
		rec->numPorts = numPorts;
		return rec;
	}
};

/*
	Helper function to compute memory alignment.
 */
template<size_t ALIGNMENT>
inline size_t align( size_t offset )
{
	return ( offset + ALIGNMENT - 1 ) & ~( ALIGNMENT - 1 );
}

// Helper template to determine the alignment of object fields.
template<typename T>
struct alignmentOf {};

// Forward declaration:
struct ObjectRecord;

// Object field of kind FK_Ref:
struct RefField
{
	co::IService* service;
	ObjectRecord* object;
};

// RefFields need pointer alignment:
template<> struct alignmentOf<RefField> { static const size_t value = sizeof(void*); };

// Object field of kind FK_RefVec:
struct RefVecField
{
	co::IService** services; // points to the start of the memory block
	ObjectRecord** objects;	 // always allocated contiguously after 'services'

	inline void allocate( size_t size )
	{
		services = reinterpret_cast<co::IService**>( malloc( sizeof(void*) * size * 2 ) );
		objects = reinterpret_cast<ObjectRecord**>( services + size );
	}

	inline size_t getSize() const
	{
		return reinterpret_cast<void**>( objects ) - reinterpret_cast<void**>( services );
	}
};

// RefVecFields need pointer alignment:
template<> struct alignmentOf<RefVecField> { static const size_t value = sizeof(void*); };

// Object field of kind FK_Value:
typedef co::Any ValueField;
 
// ValueFields need double alignment:
template<> struct alignmentOf<ValueField> { static const size_t value = sizeof(double); };

typedef std::map<co::uint16, co::uint32> SpaceRefCountMap;

/*
	An object instance record.
 */
struct ObjectRecord
{
	ComponentRecord* model;
	co::IObject* instance;

	// Number of references to this object. Equivalent to the summation of 'spaceRefs'.
	co::uint32 inDegree;

	// Number of references from this object to other objects.
	co::uint32 outDegree;

	/*
		Spaces that contain this object, with their respective reference counts.
		Potential optimization: use google-sparsehash's sparsetable?
	 */
	SpaceRefCountMap spaceRefs;

	template<typename T>
	inline T* atOffset( co::uint32 offset )
	{
		return reinterpret_cast<T*>( reinterpret_cast<co::uint8*>( this ) + offset );
	}

	inline RefField* getRef( co::uint16 i )
	{
		CORAL_STATIC_CHECK( sizeof(ObjectRecord) % sizeof(void*) == 0, alignment_assumption_failed );
		return atOffset<RefField>( sizeof(ObjectRecord) ) + i;
	}

	inline RefVecField* getRefVec( co::uint16 i )
	{
		CORAL_STATIC_CHECK( sizeof(RefField) == sizeof(RefVecField), optimization_assumption_failed );
		return atOffset<RefVecField>( sizeof(ObjectRecord) ) + model->numRefs + i;
	}

	inline ValueField* getValue( co::uint16 i )
	{
		return atOffset<ValueField>( model->offset1stValue ) + i;
	}

	static ObjectRecord* create( ComponentRecord* model, co::IObject* instance );
	void destroy();
};

/*!
	The ca.Model component.
 */
class Model : public Model_Base
{
public:
	Model();
	virtual ~Model();

	// Restricted Methods:
	RecordTypeRecord* getRecordType( co::IRecordType* recordType );
	ComponentRecord* getComponent( co::IComponent* component );

	// IModel Methods:
	const std::string& getName();
	void setName( const std::string& name );
	bool alreadyContains( co::IType* type );
	bool contains( co::IType* type );
	void getFields( co::IRecordType* recordType, co::RefVector<co::IField>& fields );
	void getPorts( co::IComponent* component, co::RefVector<co::IPort>& ports );
	void beginChanges();
	void applyChanges();
	void discardChanges();
	void addEnum( co::IEnum* enumType );
	void addRecordType( co::IRecordType* recordType, co::Range<co::IField* const> fields );
	void addComponent( co::IComponent* component, co::Range<co::IPort* const> ports );

protected:
	TypeRecord* getType( co::IType* type );
	TypeRecord* getTypeOrThrow( co::IType* type );

	bool loadCaModelFor( co::IType* type );
	void checkCanAddType( co::IType* type );

	TypeRecord* findTransactionType( co::IType* type )
	{
		TypeRecord key( type );
		TypeSet::iterator it = _transaction.find( &key );
		return it != _transaction.end() ? *it : NULL;
	}

	void validateTransaction();
	TypeRecord* validateTypeDependency( co::IType* dependency );
	void commitTransaction();

private:
	// --- permanent fields --- //

	std::string _name;

	// sorted list of types in the object model
	TypeList _types;

	// namespaces we tried to load a CaModel file from (avoids retries)
	std::set<co::INamespace*> _visitedNamespaces;

	// --- transaction fields --- //

	int _level;
	bool _discarded;
	TypeSet _transaction;
};

} // namespace ca

#endif // _CA_MODEL_H_
