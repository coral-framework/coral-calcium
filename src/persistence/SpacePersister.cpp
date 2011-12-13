/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "SpacePersister_Base.h"

#include <ca/ISpaceChanges.h>
#include <ca/IUniverse.h>
#include <ca/ISpace.h>
#include <ca/IOException.h>
#include <ca/FormatException.h>
#include <ca/ISpaceStore.h>

#include <co/RefVector.h>
#include <co/RefPtr.h>
#include <co/IObject.h>
#include <co/IField.h>
#include <co/IArray.h>
#include <co/Any.h>
#include <co/NotSupportedException.h>
#include <co/IllegalArgumentException.h>
#include <co/IllegalCastException.h>
#include <co/IllegalStateException.h>
#include <co/Log.h>

#include <map>

#include <ca/StoredType.h>
#include <ca/StoredFieldValue.h>
#include <ca/StoredField.h>

#include "StringSerializer.h"

namespace ca {

class SpacePersister : public SpacePersister_Base
{
public:
	SpacePersister()
	{
		// empty
	}

	virtual ~SpacePersister()
	{
		if( _spaceStore.isValid() )
		{
			_spaceStore->close();
		}
	}

	// ------ ca.ISpaceObserver Methods ------ //

	void onSpaceChanged( ca::ISpaceChanges* changes )
	{
		_spaceChanges.push_back( changes );
	}

	// ------ ca.ISpacePersister Methods ------ //

	void initialize( co::IObject* rootObject )
	{

		if( !_spaceStore.isValid() )
		{
			throw co::IllegalStateException("space file was not set, could not setup");
		}

		_spaceStore->open();

		try
		{
			_spaceStore->beginChanges();
			saveObject( rootObject );
			_spaceStore->endChanges();
		}
		catch(...)
		{
			_spaceStore->close();
		}

		_spaceStore->close();

	}

	ca::ISpace* getSpace()
	{
		return _space.get();
	}

	void restore()
	{
		if( !_spaceStore.isValid() )
		{
			throw co::IllegalStateException("space store was not set, can't restore a space");
		}
		restoreRevision( _spaceStore->getLatestRevision() );
	}

	void restoreRevision( co::uint32 revision )
	{
		if( !_spaceStore.isValid() )
		{
			throw co::IllegalStateException("space store was not set, can't restore a space");
		}

		_spaceStore->open();

		if( _spaceStore->getLatestRevision() == 0 )
		{
			_spaceStore->close();
			throw co::IllegalArgumentException("empty space store");
		}

		try
		{
			clear();
			co::uint32 rootObject = _spaceStore->getRootObject( revision );
			restoreObject( rootObject, revision );

			co::IObject* object = (co::IObject*)getObject( rootObject );
		
			co::IObject* spaceObj = co::newInstance( "ca.Space" );
			ca::ISpace* space = spaceObj->getService<ca::ISpace>();

			spaceObj->setService( "universe", _universe.get() );
			space->setRootObject( object );
			_spaceStore->close();
			
			_space = space;
		}
		catch( ca::IOException& e )
		{
			_spaceStore->close();
			throw e;
		}
		
	}

	void save()
	{
		co::RefPtr<ca::ISpaceChanges> current;
		for( int i = 0; i < _spaceChanges.size(); i++ )
		{
			current = _spaceChanges[ i ];
		}
	}

protected:

	ca::ISpaceStore* getStoreService()
	{
		return _spaceStore.get();
	}

	void setStoreService( ca::ISpaceStore* spaceStore )
	{
		assert( spaceStore );
		_spaceStore = spaceStore;
	}

	ca::IUniverse* getUniverseService()
	{
		return _universe.get();
	}

	void setUniverseService( ca::IUniverse* universe )
	{
		assert( universe );
		_universe = universe;
		_model = (ca::IModel*)_universe->getProvider()->getService( "model" );
	}

private:
	void observe()
	{
		_space->addSpaceObserver( this );
	}

	//save functions

	void savePort(int componentId, co::IPort* port)
	{
		int typeId = _spaceStore->getOrAddType( port->getType()->getFullName(), getCalciumModelId() );
		
		saveField(port, componentId, typeId);
	}

	void saveService(co::RefPtr<co::IService> obj, co::IPort* port)
	{
		int objId = getObjectId(obj.get());
		if(objId != 0)
		{
			return;
		}

		co::IInterface* type = port->getType();
		saveEntity(type);

		int entityId = getTypeId( obj->getInterface() );

		objId = _spaceStore->addObject( entityId );
		insertObjectCache(obj.get(), objId);

		co::Range<co::IField* const> fields = type->getFields();
						
		int j = 0;
		std::vector<ca::StoredFieldValue> values;
		std::string fieldValueStr;

		for( int i = 0; i < fields.getSize(); i++ ) 
		{
			co::IField* field =  fields[i];
			
			int fieldId = getMemberId( field );
			if( fieldId == 0 )
			{
				continue;
			}
			int fieldTypeId = getTypeId( field->getType() );
			
			co::IReflector* reflector = field->getOwner()->getReflector();
			co::Any fieldValue;

			reflector->getField(obj.get(), field, fieldValue);

			co::TypeKind tk = fieldValue.getKind();

			if(tk == co::TK_ARRAY)
			{
				co::IType* arrayType = static_cast<co::IArray*>( field->getType() )->getElementType();
										
				if(arrayType->getKind() == co::TK_INTERFACE)
				{
					co::Range<co::IService* const> services = fieldValue.get<co::Range<co::IService* const>>();
					
					std::vector<co::int32> refs;
					while(!services.isEmpty())
					{
						co::IService* const serv = services.getFirst();
						saveObject(serv->getProvider());
						
						int refId = getObjectId( serv->getProvider() );
						
						refs.push_back( refId );
						services.popFirst();
					
					}
					co::Any refsValue;
					refsValue.set<const std::vector<co::int32>&>( refs );
					_serializer.toString( refsValue, fieldValueStr );
				}
				else
				{
					_serializer.toString( fieldValue, fieldValueStr );
				}
				
			}
			else if ( tk == co::TK_INTERFACE )
			{
				co::IService* const service = fieldValue.get<co::IService* const>();
				if(service == NULL)
				{
					fieldValueStr = "nil";
				}
				else
				{
					saveObject( service->getProvider() );
					_serializer.toString( getObjectId(service->getProvider()), fieldValueStr );
				}
				
			}
			else
			{
				_serializer.toString( fieldValue, fieldValueStr );
			}

			ca::StoredFieldValue value;
			value.fieldId = fieldId;
			value.value = fieldValueStr;
			values.push_back( value );
		}
		_spaceStore->addValues( objId, values );
	}

	bool needsToSaveType( co::IType* type )
	{
		return ( getTypeId( type ) == 0 );
	}

	void saveEntity( co::IType* type )
	{
		if( !needsToSaveType(type) )
		{
			return;
		}

		co::TypeKind tk = type->getKind();

		if( tk == co::TK_COMPONENT )
		{
			co::IComponent*	component = static_cast<co::IComponent*>( type );
			saveComponent( component );
			return;
		}

		if( tk == co::TK_INTERFACE )
		{
			co::IInterface*	interfaceType = static_cast<co::IInterface*>(type);
			saveInterface( interfaceType );
			return;
		}
			
		int typeId = _spaceStore->getOrAddType( type->getFullName(), getCalciumModelId() );
		insertTypeCache( type, typeId );
	}

	void saveField(co::IMember* member, int entityId, int fieldTypeId)
	{
		int fieldId = _spaceStore->addField( entityId, member->getName(), fieldTypeId );
		insertMemberCache( member, fieldId );

	}

	void saveInterface(co::IInterface* entityInterface)
	{

		int getIdFromSavedEntity = _spaceStore->getOrAddType( entityInterface->getFullName(), getCalciumModelId() );
		insertTypeCache( entityInterface, getIdFromSavedEntity );

		co::RefVector<co::IField> fields;
		_model->getFields(entityInterface, fields);
		
		for(co::RefVector<co::IField>::iterator it = fields.begin(); it != fields.end(); it++) 
		{
			co::IField* field =  it->get();
			
			int fieldTypeId = getTypeId( field->getType() );

			if( fieldTypeId == 0 )
			{
				saveEntity( field->getType() );
				fieldTypeId = getTypeId( field->getType() );
			}
			
			saveField(field, getIdFromSavedEntity, fieldTypeId);

		}

	}

	void saveObject(co::RefPtr<co::IObject> object)
	{
		if(getObjectId(object.get()) != 0)
		{
			return;
		}

		co::IComponent* component = object->getComponent();

		std::string name = component->getFullName();
		saveEntity(component);

		int entityId = getTypeId( component );
		int objId = _spaceStore->addObject( entityId );

		insertObjectCache(object.get(), objId);

		co::Range<co::IPort* const> ports = component->getPorts();

		int fieldId;
		std::vector<ca::StoredFieldValue> values;
		for( int i = 0; i < ports.getSize(); i++ )
		{
			co::IPort* port = (ports[i]);
			fieldId = getMemberId( port );
			
			if( fieldId == 0 )
			{
				continue;
			}
			
			co::IService* service = object->getServiceAt(port);
			
			std::string refStr;
			if(port->getIsFacet())
			{
				saveService(service, port);
				std::stringstream insertValue;
				
				_serializer.toString( getObjectId( service ), refStr);
			}
			else
			{
				_serializer.toString( getObjectId( service->getProvider() ), refStr);
			}
			ca::StoredFieldValue value;
			value.fieldId = fieldId;
			value.value = refStr;
			values.push_back( value );
			
		}
		_spaceStore->addValues( objId, values );

	}

	void saveComponent(co::IComponent* component)
	{
		int componentId = _spaceStore->getOrAddType( component->getFullName(), getCalciumModelId() );
		insertTypeCache( component, componentId );

		co::RefVector< co::IPort > ports;
		_model->getPorts( component, ports );

		for( int i = 0; i < ports.size(); i++ )
		{
			savePort( componentId, ports[i].get() );
		}
	}

	//restore functions

	void restoreObject( int id, int revision )
	{
		int typeId = _spaceStore->getObjectType( id );

		co::IType* type = getType( typeId );

		std::string entity = type->getFullName();
		
		co::IObject* object = co::newInstance( entity );
		
		co::Range<co::IPort* const> ports = object->getComponent()->getPorts();

		std::vector<ca::StoredFieldValue> fieldValues;
		_spaceStore->getValues( id, revision, fieldValues );
		
		FieldValueMap mapFieldValue;

		for( int i = 0; i < fieldValues.size(); i++ )
		{
			ca::StoredFieldValue fieldValue = fieldValues[i];
			mapFieldValue.insert( FieldValueMap::value_type( fieldValue.fieldId, fieldValue.value ) );
		}

		insertObjectCache( object, id );

		co::IPort* port;
		for( int i = 0; i < ports.getSize(); i++ )
		{
			port = ports[i];
			
			int fieldId = getMemberId( port );
			if( fieldId == 0 )
			{
				continue;
			}

			int idService = atoi( mapFieldValue.find(fieldId)->second.c_str() );

			if( port->getIsFacet() )
			{
				co::IService* service = object->getServiceAt( port );
				fillServiceValues( idService, service, revision );
			}
			else 
			{
				co::IObject* refObj = (co::IObject*)getObject( idService, revision );
				if( refObj == NULL )
				{
					throw co::IllegalStateException();
				}
				try
				{
					object->setServiceAt( port, refObj->getServiceAt( getFirstFacet( refObj, port->getType() ) ) );
				}
				catch( co::IllegalCastException& )
				{
					std::stringstream ss;
					ss << "Could not restore object, invalid value type for port " << port->getName() << "on entity " << entity;
					throw ca::IOException( ss.str() );
				}
			}

		}
		
	}

	co::IPort* getFirstFacet( co::IObject* refObj, co::IType* type )
	{
		co::RefVector< co::IPort > ports;
		_model->getPorts( refObj->getComponent(), ports );

		for( int i = 0; i < ports.size(); i++ )
		{
			if( ports[i].get()->getIsFacet() && (ports[i]->getType()->getFullName() == type->getFullName()) )
			{
				return ports[i].get();
			}
		}
		return NULL;
	}

	void fillServiceValues( int id, co::IService* service, int revision )
	{

		std::vector<ca::StoredFieldValue> fieldValues;

		int typeId = _spaceStore->getOrAddType( service->getInterface()->getFullName(), getCalciumModelId() );
		co::IService* type = (co::IService*)getType( typeId );

		_spaceStore->getValues( id, revision, fieldValues );
		FieldValueMap mapFieldValue;

		for( int i = 0; i < fieldValues.size(); i++ )
		{
			ca::StoredFieldValue fieldValue = fieldValues[i];
			mapFieldValue.insert( FieldValueMap::value_type( fieldValue.fieldId, fieldValue.value ) );
		}

		co::Range<co::IField* const> fields = service->getInterface()->getFields();
		
		co::IReflector* ref = service->getInterface()->getReflector();
		for( int i = 0; i < fields.getSize(); i++)
		{
			co::IField* field = fields[i];
			int fieldId = getMemberId( field );
			if( fieldId == 0 )
			{
				continue;
			}

			fillFieldValue(service, field, id, mapFieldValue.find(fieldId)->second);
		}
	}

	void fillFieldValue( co::IService* service, co::IField* field, int id, std::string strValue )
	{
		co::IType* type = field->getType();
		co::IReflector* reflector = service->getInterface()->getReflector();

		co::Any serviceAny;
		serviceAny.setService( service, service->getInterface() );

		co::Any fieldValue;

		if( type->getKind() == co::TK_ARRAY )
		{
			co::IArray* arrayType = static_cast<co::IArray*>(type);

			if( arrayType->getElementType()->getKind() == co::TK_INTERFACE )
			{
				fillInterfaceArrayValue(strValue, arrayType, fieldValue);
			}
		
		}
		if( type->getKind() == co::TK_INTERFACE )
		{
			if( strValue != "nil" )
			{
				co::Any refId;
				_serializer.fromString( strValue, co::typeOf<co::int32>::get(), refId );
				
				int refIdInt = refId.get<co::int32>();
				co::IObject* ref = (co::IObject*)getObject( refIdInt, 1 );
				if( ref == NULL )
				{
					throw co::IllegalStateException();
				}

				fieldValue.set(ref->getServiceAt( getFirstFacet(ref, field->getType()) ));
			}

		}
		else
		{
			try
			{
				_serializer.fromString(strValue, field->getType(), fieldValue);
			}
			catch( ca::FormatException& )
			{
				std::stringstream ss;
				ss << "Invalid field value for field " << field->getName();
				throw ca::FormatException( ss.str() );
			}
			
		}
		
		try
		{
			if( fieldValue.getKind() != co::TK_NONE )
			{
				reflector->setField( serviceAny, field, fieldValue );
			}

		}
		catch (co::IllegalCastException&)
		{
			std::stringstream ss;
			ss << "Invalid field value type for field " << field->getName() << "; type expected: " << field->getType()->getFullName();
			throw ca::IOException( ss.str() );
		}

	}

	void fillInterfaceArrayValue( std::string strValue, co::IArray* arrayType, co::Any& arrayValue )
	{
		assert( arrayType->getElementType()->getKind() == co::TK_INTERFACE );

		co::Any refs;
		_serializer.fromString(strValue, co::typeOf<std::vector<co::int32>>::get(), refs);

		std::vector<co::int32> vec = refs.get<const std::vector<co::int32>&>();
		co::RefVector<co::IService> services;

		arrayValue.createArray( arrayType->getElementType(), vec.size() );

		co::IObject* ref;
		for( int i = 0; i < vec.size(); i++ )
		{
			ref = (co::IObject*)getObject( vec[i], 1 );
			if( ref == NULL )
			{
				throw co::IllegalStateException();
			}
			co::IPort* port = getFirstFacet(ref, arrayType->getElementType());	
			co::IService* service = ref->getServiceAt(port);
						
			co::Any servicePtr;
			servicePtr.setService(service);
						
			try
			{
				AnyArrayUtil arrayUtil;
				arrayUtil.setArrayComplexTypeElement( arrayValue, i, servicePtr );
			}
			catch ( co::NotSupportedException& )
			{
				throw ca::IOException( "Could not restore array" );
			}
		}

	}

	int getCalciumModelId()
	{
		return 1;
	}

	co::IObject* getObject( co::uint32 objectId, co::uint32 revision )
	{
		co::IObject* object = (co::IObject*) getObject( objectId );
		
		if( object == NULL )
		{
			restoreObject( objectId, revision );
			co::IService* service = getObject( objectId );
			if(service == NULL)
			{
				throw co::IllegalStateException();
			}
			object = (co::IObject*)service;
		}
		return object;
	}

	co::IType* getType( co::uint32 id )
	{
		co::IType* resultType = getCachedType( id );

		if( resultType == NULL )
		{
			StoredType storedType;
			_spaceStore->getType( id, storedType );

			resultType = co::getType( storedType.typeName );

			insertTypeCache( resultType, id );
			
			if( resultType->getKind() == co::TK_COMPONENT || resultType->getKind() == co::TK_INTERFACE )
			{
				co::ICompositeType* compositeType = (co::ICompositeType*)resultType;
				
				std::string storedFieldName;
				for( int i = 0; i < storedType.fields.size(); i++ )
				{
					storedFieldName = storedType.fields[i].fieldName;
					co::IMember* member = compositeType->getMember( storedFieldName );
					assert( member != NULL );
					insertMemberCache( member, storedType.fields[i].fieldId );
				} 
			}
		}

		return resultType;

	}

	co::uint32 getTypeId( co::IType* type )
	{
		TypeIdMap::iterator it = _typeIdCache.find( type );

		if(it != _typeIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	co::IType* getCachedType( co::uint32 id )
	{
		IdTypeMap::iterator it = _typeCache.find(id);

		if(it != _typeCache.end())
		{
			return it->second;
		}

		return NULL;

	}

	co::IMember* getMember( co::uint32 id )
	{
		IdMemberMap::iterator it = _memberCache.find(id);

		if(it != _memberCache.end())
		{
			return it->second;
		}

		return NULL;

	}

	co::uint32 getMemberId( co::IMember* member )
	{
		MemberIdMap::iterator it = _memberIdCache.find( member );

		if(it != _memberIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	co::IService* getObject( co::uint32 id )
	{
		IdObjectMap::iterator it = _objectCache.find(id);

		if(it != _objectCache.end())
		{
			return it->second;
		}

		return NULL;

	}

	co::uint32 getObjectId(co::IService* obj)
	{
		ObjectIdMap::iterator it = _objectIdCache.find(obj);

		if(it != _objectIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	//generates two way mapping
	void insertObjectCache( co::IService* obj, co::uint32 id )
	{
		_objectIdCache.insert(ObjectIdMap::value_type( obj, id ));
		_objectCache.insert(IdObjectMap::value_type( id, obj ));
	}

	void insertTypeCache( co::IType* type, co::uint32 id )
	{
		_typeIdCache.insert( TypeIdMap::value_type( type, id ) );
		_typeCache.insert( IdTypeMap::value_type( id, type ) );
	}

	void insertMemberCache( co::IMember* member, co::uint32 id )
	{
		_memberIdCache.insert( MemberIdMap::value_type( member, id ) );
		_memberCache.insert( IdMemberMap::value_type( id, member ) );
	}

	void clear()
	{
		_objectCache.clear();
		_objectIdCache.clear();

		_typeCache.clear();
		_typeIdCache.clear();

		_memberCache.clear();
		_memberIdCache.clear();
	}
private:

	co::RefPtr<ca::ISpace> _space;
	ca::IModel* _model;

	StringSerializer _serializer;

	co::RefPtr<ca::IUniverse> _universe;
	co::RefPtr<ca::ISpaceStore> _spaceStore;

	co::RefVector<ca::ISpaceChanges> _spaceChanges;

	typedef std::map<co::uint32, std::string> FieldValueMap;
	typedef std::map<co::IService*, co::uint32> ObjectIdMap;
	typedef std::map<co::uint32, co::IService*> IdObjectMap;

	typedef std::map<co::IType*, co::uint32> TypeIdMap;
	typedef std::map<co::uint32, co::IType*> IdTypeMap;

	typedef std::map<co::IMember*, co::uint32> MemberIdMap;
	typedef std::map<co::uint32, co::IMember*> IdMemberMap;

	ObjectIdMap _objectIdCache;
	IdObjectMap _objectCache;

	TypeIdMap _typeIdCache;
	IdTypeMap _typeCache;

	MemberIdMap _memberIdCache;
	IdMemberMap _memberCache;
};

CORAL_EXPORT_COMPONENT( SpacePersister, SpacePersister );

} // namespace ca
