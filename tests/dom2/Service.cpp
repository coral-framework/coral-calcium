#include "Service_Base.h"
#include <dom/IService.h>
#include <dom/IEmployee.h>

namespace dom {

class Service : public Service_Base
{
public:
	Service()
	{
		// empty
	}

	virtual ~Service()
	{
		// empty
	}

	std::string getName() { return _name; }
	void setName( const std::string& name ) { _name = name; }

	double getMonthlyIncome() { return _earnings; }
	void setMonthlyIncome( double earnings ) { _earnings = earnings; }

	co::TSlice<IEmployee*> getMantainers()
	{
		return _developers;
	}

	void setMantainers( co::Slice<IEmployee*> developers )
	{
		co::assign( developers, _developers );
	}

private:
	std::string _name;
	double _earnings;
	std::vector<IEmployeeRef> _developers;
};

CORAL_EXPORT_COMPONENT( Service, Service )

} // namespace erm
