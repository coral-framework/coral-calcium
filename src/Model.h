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
	Kinds of types that can be added to a calcium model.
 */
enum TypeRecordKind
{
	TRK_ENUM,		// enum
	TRK_RECORD,		// struct or native class
	TRK_INTERFACE,	// interface
	TRK_COMPONENT	// component
};

/*
	Base record for a type in the object model. Only directly used for enums.
 */
struct TypeRecord
{
	co::IType* type;
	TypeRecordKind kind;

	inline TypeRecord( co::IType* type ) : type( type )
	{;}

	inline bool isEnum() const { return kind == TRK_ENUM; }
	inline bool isRecord() const { return kind == TRK_RECORD; }
	inline bool isInterface() const { return kind == TRK_INTERFACE; }
	inline bool isComponent() const { return kind == TRK_COMPONENT; }

	inline void destroy() { free( this ); }

	inline void init( co::IType* type, TypeRecordKind kind )
	{
		this->type = type;
		this->kind = kind;
	}

	static TypeRecord* create( co::IEnum* type );
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
	Record for a struct or native class in the object model.

	Field records (in the 'fields' array) are sorted/grouped by FieldKind:
		- The first 'numRefs' fields in the array are FK_Ref's.
		- The following 'numRefVecs' fields are FK_RefVec's.
		- The final 'numValues' fields are FK_Value's.
 */
struct RecordRecord : TypeRecord
{
	co::uint16 numFields;	// number of elements in 'fields'
	co::IField* fields[1];	// array of fields

	inline void addField( co::IField* field ) { fields[numFields++] = field; }

	static RecordRecord* create( co::IRecordType* type, co::uint16 numFields );
};

// Represents a field within an InterfaceRecord.
struct FieldRecord
{
	co::IField* field; // field descriptor
	co::uint32 offset; // position of the field's memory area within its facet

	inline co::IReflector* getTypeReflector() const
	{
		return field->getType()->getReflector();
	}

	inline co::IReflector* getOwnerReflector() const
	{
		return field->getOwner()->getReflector();
	}

	inline co::uint32 getSize() const
	{
		return getTypeReflector()->getSize();
	}
};

// Forward declaration:
struct ObjectRecord;

// Object field of kind FK_Ref:
struct RefField
{
	co::IService* service;
	ObjectRecord* object;
};

// Object field of kind FK_RefVec:
struct RefVecField
{
	co::IService** services; // points to the start of the memory block
	ObjectRecord** objects;	 // always allocated contiguously after 'services'

	inline void create( size_t size )
	{
		services = reinterpret_cast<co::IService**>( malloc( sizeof(void*) * size * 2 ) );
		objects = reinterpret_cast<ObjectRecord**>( services + size );
	}

	inline void destroy()
	{
		free( services );
		services = NULL;
		objects = NULL;
	}

	inline size_t getSize() const
	{
		return reinterpret_cast<void**>( objects ) - reinterpret_cast<void**>( services );
	}
};

/*
	Record for an interface in the object model.

	The 'fields' array is sorted by FieldKind:
		- The first 'numRefs' fields in the array are FK_Ref's.
		- The following 'numRefVecs' fields are FK_RefVec's.
		- The final 'numValues' fields are FK_Value's.
 */
struct InterfaceRecord : TypeRecord
{
	co::uint32 size;		// number of bytes required to store all fields

	co::uint16 numRefs;		// references are in fields[0..numRefs-1]
	co::uint16 numRefVecs;	// ref-vecs are in fields[numRefs..firstValue-1]
	co::uint16 numValues;	// values are in fields[firstValue..numFields-1]

	co::uint16 firstValue;	// numRefs + numRefVecs
	co::uint16 numFields;	// numRefs + numRefVecs + numValues

	FieldRecord fields[1];

	void addField( FieldKind fieldKind, co::IField* field );

	/*
		Must be called only after all fields have been added.
		This re-orders the value fields by descending size, and allocates
		all fields (guaranteeing 8-byte alignment for values >= 8 bytes).
	 */
	void computeLayout();

	static InterfaceRecord* create( co::IInterface* type, co::uint16 numFields );
};

/*
	Represents a port within a ComponentRecord.
 */
struct PortRecord
{
	co::IPort* port;			// identifies the port within the component
	co::uint32 offset;			// offset of the port's memory area within an object
	InterfaceRecord* typeRec;	// only relevant for facets
};

/*
	ComponentRecords are blueprints for calcium objects.

	The set of receptacles and facets defined for a component
	determines its memory layout.
 */
struct ComponentRecord : TypeRecord
{
	co::uint8 numFacets;		// facets are in ports[0..numFacets-1]
	co::uint8 numReceptacles;	// receptacles are in ports[numFacets..numPorts-1]
	co::uint8 numPorts;			// numReceptacles + numFacets

	co::uint32 objectSize;		// total number of bytes needed for an object instance

	// Facets are grouped at the start, receptacles at the end
	PortRecord ports[1];

	void addPort( co::IPort* port );

	// Must be called only after all ports have been initialized with their typeRec.
	void computeLayout();

	static ComponentRecord* create( co::IComponent* type, co::uint8 numPorts );
};

typedef std::map<co::uint16, co::uint32> SpaceRefCountMap;

/*
	Record for a calcium object instance.
 
	The memory of a calcium object is organized as follows:
		[ObjectRecord]	// Meta-object data (see the ObjectRecord struct).
		[Facet Refs]	// Array of IServices for the facets.
		[Receptacles]	// Array of RefFields for the receptacles.
		[Facet Data #1]	// All stored fields from facet #1.
		[Facet Data #2]	// All stored fields from facet #2.
		...				// etc.
 
	Each facet data block is organized as follows:
		[References]	// Single reference fields (RefField).
		[RefVectors]	// Reference vector fields (RefVecField).
		[Values]		// Value fields (aligned/packed memory blocks).
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

	// Facet Refs: pointers to the services provided by this object
	co::IService* services[1];

	template<typename T>
	inline T* get( co::uint32 atOffset )
	{
		return reinterpret_cast<T*>( reinterpret_cast<co::uint8*>( this ) + atOffset );
	}

	inline RefField* getReceptacles()
	{
		return get<RefField>( model->ports[model->numFacets].offset );
	}

	void destroy();

	static ObjectRecord* create( ComponentRecord* model, co::IObject* instance );
};

// Traverses all receptacles in an object using a callback.
template<typename Callback>
void traverseReceptacles( ObjectRecord* object, Callback& cb )
{
	ComponentRecord* model = object->model;
	PortRecord* ports = &model->ports[model->numFacets];
	RefField* refs = object->getReceptacles();
	for( co::uint8 i = 0; i < model->numReceptacles; ++i )
		cb( object, ports[i], refs[i] );
}

// Traverses all Ref fields in an object's facet using a callback.
template<typename Callback>
void traverseFacetRefs( ObjectRecord* object, co::uint8 facetId, PortRecord& facet, Callback& cb )
{
	InterfaceRecord* itf = facet.typeRec;
	FieldRecord* fields = &itf->fields[0];
	RefField* refs = object->get<RefField>( facet.offset + 0 );
	for( co::uint16 i = 0; i < itf->numRefs; ++i )
		cb( object, facetId, fields[i], refs[i] );
}

//! Traverses all RefVec fields in an object's facet using a callback.
template<typename Callback>
void traverseFacetRefVecs( ObjectRecord* object, co::uint8 facetId, PortRecord& facet, Callback& cb )
{
	InterfaceRecord* itf = facet.typeRec;
	FieldRecord* fields = &itf->fields[itf->numRefs];
	RefVecField* refVecs = object->get<RefVecField>( facet.offset + sizeof(RefField) * itf->numRefs );
	for( co::uint16 i = 0; i < itf->numRefVecs; ++i )
		cb( object, facetId, fields[i], refVecs[i] );
}

//! Traverses all Value fields in an object's facet using a callback.
template<typename Callback>
void traverseFacetValues( ObjectRecord* object, co::uint8 facetId, PortRecord& facet, Callback& cb )
{
	InterfaceRecord* itf = facet.typeRec;
	FieldRecord* fields = &itf->fields[itf->firstValue];
	for( co::uint16 i = 0; i < itf->numValues; ++i )
		cb( object, facetId, fields[i], object->get<void>( facet.offset + fields[i].offset ) );
}

// Traverses all fields in an object's facet using a callback.
template<typename Callback>
void traverseFacet( ObjectRecord* object, co::uint8 facetId, Callback& cb )
{
	PortRecord& facet = object->model->ports[facetId];

	if( facet.typeRec->numRefs > 0 )
		traverseFacetRefs( object, facetId, facet, cb );

	if( facet.typeRec->numRefVecs > 0 )
		traverseFacetRefVecs( object, facetId, facet, cb );

	if( facet.typeRec->numValues > 0 )
		traverseFacetValues( object, facetId, facet, cb );
}

// Traverses all object receptacles and fields (of all kinds).
template<typename Callback>
void traverseObject( ObjectRecord* object, Callback& cb )
{
	traverseReceptacles( object, cb );

	co::uint8 numFacets = object->model->numFacets;
	for( co::uint8 i = 0; i < numFacets; ++i )
		traverseFacet( object, i, cb );
}

// Traverses all object receptacles and fields of Ref/RefVec kind.
template<typename Callback>
void traverseObjectRefs( ObjectRecord* object, Callback& cb )
{
	traverseReceptacles( object, cb );

	ComponentRecord* model = object->model;
	for( co::uint8 i = 0; i < model->numFacets; ++i )
	{
		PortRecord& facet = model->ports[i];
		if( facet.typeRec->numRefs > 0 )
			traverseFacetRefs( object, i, facet, cb );

		if( facet.typeRec->numRefVecs > 0 )
			traverseFacetRefVecs( object, i, facet, cb );
	}
}

/*!
	The ca.Model component.
 */
class Model : public Model_Base
{
public:
	Model();
	virtual ~Model();

	// Restricted Methods:
	inline RecordRecord* getRecord( co::IRecordType* type )
	{
		TypeRecord* rec = getTypeOrThrow( type );
		assert( rec->isRecord() );
		return static_cast<RecordRecord*>( rec );
	}

	inline InterfaceRecord* getInterface( co::IInterface* type )
	{
		TypeRecord* rec = getTypeOrThrow( type );
		assert( rec->isInterface() );
		return static_cast<InterfaceRecord*>( rec );
	}

	inline ComponentRecord* getComponent( co::IComponent* type )
	{
		TypeRecord* rec = getTypeOrThrow( type );
		assert( rec->isComponent() );
		return static_cast<ComponentRecord*>( rec );
	}

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
