#include "Company_Base.h"
#include <dom/IDeveloper.h>
#include <dom/IManager.h>
#include <dom/IProject.h>
#include <dom/IDeveloper.h>
#include <co/Exception.h>

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

	co::TSlice<IDeveloper*> getDevelopers()
	{
		return _developers;
	}

	void setDevelopers( co::Slice<IDeveloper*> developers )
	{
		co::assign( developers, _developers );
	}

	co::TSlice<IManager*> getManagers()
	{
		return _managers;
	}

	void setManagers( co::Slice<IManager*> managers )
	{
		co::assign( managers, _managers );
	}
	
	co::TSlice<IProject*> getProjects()
	{
		return _projects;
	}

	void setProjects( co::Slice<IProject*> projects )
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
	std::vector<IDeveloperRef> _developers;
	std::vector<IManagerRef> _managers;
	std::vector<IProjectRef> _projects;
};
	
CORAL_EXPORT_COMPONENT( Company, Company )

} // namespace erm
