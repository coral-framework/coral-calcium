#include "Employee_Base.h"
#include <dom/IProject.h>

namespace dom {

class Employee : public Employee_Base
{
public:
	Employee()
	{
		// empty
	}

	virtual ~Employee()
	{
		// empty
	}

	std::string getRole()
	{
		return _role;
	}

	void setRole( const std::string& role )
	{
		_role = role;
	}

	co::TSlice<IProject*> getWorking()
	{
		return _working;
	}

	void setWorking( co::Slice<IProject*> working )
	{
		co::assign( working, _working );
	}

	IProject* getLeading() { return _leading.get(); }
	void setLeading( IProject* leading ) { _leading = leading; }

	std::string getName() { return _name; }
	void setName( const std::string& name ) { _name = name; }

	co::int32 getSalary() { return _salary; }
	void setSalary( co::int32 salary ) { _salary = salary; }

private:
	IProjectRef _leading;
	std::vector<IProjectRef> _working;
	std::string _name;
	std::string _role;
	co::int32 _salary;
};

CORAL_EXPORT_COMPONENT( Employee, Employee )

} // namespace erm
