#include "Developer_Base.h"
#include <dom/IProject.h>

namespace dom {

class Developer : public Developer_Base
{
public:
	Developer() : _project( NULL )
	{
		// empty
	}

	virtual ~Developer()
	{
		// empty
	}

	IProject* getProject() { return _project; }
	void setProject( IProject* project ) { _project = project; }

	const std::string& getName() { return _name; }
	void setName( const std::string& name ) { _name = name; }
	
	double getSalary() { return _salary; }
	void setSalary( double salary ) { _salary = salary; }

protected:


private:
	IProject* _project;
	std::string _name;
	double _salary;

};

CORAL_EXPORT_COMPONENT( Developer, Developer )

} // namespace erm
