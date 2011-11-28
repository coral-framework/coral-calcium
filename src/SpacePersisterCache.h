#ifndef _SPACE_PERSISTER_CACHE_
#define _SPACE_PERSISTER_CACHE_

#include <co/IField.h>
#include <co/IObject.h>
#include <co/IType.h>

#include <string.h>
using namespace std;

class SpacePersisterCache 
{
public:
	co::uint32 getTypeId( co::IType* type )
	{
		map<co::IType*, co::uint32>::iterator it = _typeIdCache.find( type );

		if(it != _typeIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	co::IType* getType( co::uint32 id )
	{
		map<co::uint32, co::IType*>::iterator it = _typeCache.find(id);

		if(it != _typeCache.end())
		{
			return it->second;
		}
		
		return NULL;

	}

	co::IMember* getMember( co::uint32 id )
	{
		map<co::uint32, co::IMember*>::iterator it = _memberCache.find(id);

		if(it != _memberCache.end())
		{
			return it->second;
		}
		
		return NULL;

	}

	co::uint32 getMemberId( co::IMember* member )
	{
		map<co::IMember*, co::uint32>::iterator it = _memberIdCache.find( member );

		if(it != _memberIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	co::IService* getObject( co::uint32 id )
	{
		map<co::uint32, co::IService*>::iterator it = _objectCache.find(id);

		if(it != _objectCache.end())
		{
			return it->second;
		}
		
		return NULL;

	}

	co::uint32 getObjectId(co::IService* obj)
	{
		map<co::IService*,co::uint32>::iterator it = _objectIdCache.find(obj);

		if(it != _objectIdCache.end())
		{
			return it->second;
		}

		return 0;
	}

	//generates two way mapping
	void insertObjectCache( co::IService* obj, co::uint32 id )
	{
		_objectIdCache.insert(pair<co::IService*, co::uint32>( obj, id ));
		_objectCache.insert(pair<co::uint32, co::IService*>( id, obj ));
	}

	void insertTypeCache( co::IType* type, co::uint32 id )
	{
		_typeIdCache.insert(pair<co::IType*, co::uint32>( type, id ));
		_typeCache.insert(pair<co::uint32, co::IType*>( id, type ));
	}

	void insertMemberCache( co::IMember* member, co::uint32 id )
	{
		_memberIdCache.insert(pair<co::IMember*, co::uint32>(member, id));
		_memberCache.insert(pair<co::uint32, co::IMember*>(id, member));
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

	map<co::IService*, co::uint32> _objectIdCache;
	map<co::uint32, co::IService*> _objectCache;

	map<co::IType*, co::uint32> _typeIdCache;
	map<co::uint32, co::IType*> _typeCache;

	map<co::IMember*, co::uint32> _memberIdCache;
	map<co::uint32, co::IMember*> _memberCache;
};

#endif