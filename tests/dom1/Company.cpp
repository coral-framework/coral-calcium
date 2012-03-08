#include "Company_Base.h"
#include <dom/IDeveloper.h>
#include <dom/IManager.h>
#include <dom/IProject.h>
#include <dom/IDeveloper.h>
#include <co/Exception.h>
#include <co/RefVector.h>

namespace dom {

class Company : public Company_Base
{
public:
	Company()
	{
		// empty
	}

	virtual ~Company()
	{
		// empty
	}

	co::Range<IDeveloper* const> getDevelopers()
	{
		return _developers;
	}

	void setDevelopers( co::Range<IDeveloper* const> developers )
	{
		co::assign( developers, _developers );
	}

	co::Range<IManager* const> getManagers()
	{
		return _managers;
	}

	void setManagers( co::Range<IManager* const> managers )
	{
		co::assign( managers, _managers );
	}
	
	co::Range<IProject* const> getProjects()
	{
		return _projects;
	}

	void setProjects( co::Range<IProject* const> projects )
	{
		co::assign( projects, _projects );
	}

	void addDeveloper( IDeveloper* developer  )
	{
		_developers.push_back( developer );
	}

	void addManager( IManager* manager  )
	{
		_managers.push_back( manager );
	}

	void addProject( IProject* project  )
	{
		_projects.push_back( project );
	}

private:
	co::RefVector<IDeveloper> _developers;
	co::RefVector<IManager> _managers;
	co::RefVector<IProject> _projects;
};
	
CORAL_EXPORT_COMPONENT( Company, Company )

} // namespace erm
