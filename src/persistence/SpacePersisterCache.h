#ifndef _SPACE_PERSISTER_CACHE_
#define _SPACE_PERSISTER_CACHE_

#include <co/IMember.h>
#include <co/IService.h>
#include <co/IType.h>

#include <string.h>
#include <map>

namespace ca {

class SpacePersisterCache 
{
public:
	co::uint32 getTypeId( co::IType* type )
	{
		TypeIdMap::iterator it = _typeIdCache.find( type );

		if(it != _typeIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	co::IType* getType( co::uint32 id )
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

} // namespace ca

#endif