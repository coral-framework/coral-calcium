/*
 * Calcium Object Model Framework
 * See copyright notice in LICENSE.md
 */

#include "ca_Base.h"
#include "ModuleInstaller.h"
#include <co/Coral.h>
#include <co/System.h>
#include <co/ModuleManager.h>

namespace ca {

/*!
	The ca module's co.ModulePart.
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

	void initialize( co::Module* module )
	{
		co::getSystem()->getModules()->load( "lua" );

		ca::ModuleInstaller::instance().install();
	}

	void integrate( co::Module* )
	{
		// empty
	}

	void integratePresentation( co::Module* )
	{
		// empty
	}

	void disintegrate( co::Module* )
	{
		// empty
	}

	void dispose( co::Module* )
	{
		ca::ModuleInstaller::instance().uninstall();
	}
};

CORAL_EXPORT_MODULE_PART( ModulePart );

} // namespace ca

CORAL_EXPORT_COMPONENT( ca::ModulePart, ca );
