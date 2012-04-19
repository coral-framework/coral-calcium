/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ca_Base.h"
#include "ModuleInstaller.h"
#include <co/Coral.h>
#include <co/ISystem.h>
#include <co/IModuleManager.h>
#include <co/IServiceManager.h>
#include <ca/ILuaManager.h>
#include <lua/IState.h>
#include <lua/IInterceptor.h>

namespace ca {

/*!
	The ca module's co.IModulePart.
 */
class ModulePart : public ca::ca_Base
{
public:
    ModulePart()
	{;}

	virtual ~ModulePart()
	{;}

	void initialize( co::IModule* module )
	{
		co::getSystem()->getModules()->load( "lua" );

		ca::ModuleInstaller::instance().install();
	}

	void integrate( co::IModule* )
	{
		co::RefPtr<co::IObject> luaManager = co::newInstance( "ca.LuaManager" );

		co::getSystem()->getServices()->addService(
			co::typeOf<ca::ILuaManager>::get(),
			luaManager->getService<ca::ILuaManager>() );

		_luaInterceptor = luaManager->getService<lua::IInterceptor>();
		co::getService<lua::IState>()->addInterceptor( _luaInterceptor.get() );
	}

	void integratePresentation( co::IModule* )
	{
		// empty
	}

	void disintegrate( co::IModule* )
	{
		co::getService<lua::IState>()->removeInterceptor( _luaInterceptor.get() );
		co::getSystem()->getServices()->removeService( co::typeOf<ca::ILuaManager>::get() );
		_luaInterceptor = NULL;
	}

	void dispose( co::IModule* )
	{
		ca::ModuleInstaller::instance().uninstall();
	}

private:
	co::RefPtr<lua::IInterceptor> _luaInterceptor;
};

CORAL_EXPORT_MODULE_PART( ModulePart );
CORAL_EXPORT_COMPONENT( ModulePart, ca );

} // namespace ca
