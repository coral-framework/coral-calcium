#include "Product_Base.h"
#include <dom/IProduct.h>
#include <dom/IEmployee.h>

namespace dom {

class Product : public Product_Base
{
public:
	Product()
	{
		// empty
	}

	virtual ~Product()
	{
		// empty
	}

	std::string getName() { return _name; }
	void setName( const std::string& name ) { _name = name; }

	double getValue() { return _earnings; }
	void setValue( double earnings ) { _earnings = earnings; }

	co::TSlice<IEmployee*> getDevelopers()
	{
		return _developers;
	}

	void setDevelopers( co::Slice<IEmployee*> developers )
	{
		co::assign( developers, _developers );
	}

	IEmployee* getLeader()
	{
		return _leader.get();
	}

	void setLeader( IEmployee* leader )
	{
		_leader = leader;
	}

protected:


private:
	std::string _name;
	double _earnings;
	std::vector<IEmployeeRef> _developers;
	IEmployeeRef _leader;
};

CORAL_EXPORT_COMPONENT( Product, Product )

} // namespace erm
