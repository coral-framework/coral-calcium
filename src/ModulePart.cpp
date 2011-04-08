/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ca_Base.h"
#include "ModuleInstaller.h"
#include <co/Coral.h>
#include <co/ISystem.h>
#include <co/IModuleManager.h>

namespace ca {

/*!
	The ca module's co.IModulePart.
 */
class ModulePart : public ca::ca_Base
{
public:
    ModulePart()
	{
		// empty
	}

	virtual ~ModulePart()
	{
		// empty
	}

	void initialize( co::IModule* module )
	{
		co::getSystem()->getModules()->load( "lua" );

		ca::ModuleInstaller::instance().install();
	}

	void integrate( co::IModule* )
	{
		// empty
	}

	void integratePresentation( co::IModule* )
	{
		// empty
	}

	void disintegrate( co::IModule* )
	{
		// empty
	}

	void dispose( co::IModule* )
	{
		ca::ModuleInstaller::instance().uninstall();
	}
};

CORAL_EXPORT_MODULE_PART( ModulePart );
CORAL_EXPORT_COMPONENT( ModulePart, ca );

} // namespace ca
