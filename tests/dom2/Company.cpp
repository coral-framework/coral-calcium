#include "Company_Base.h"
#include <dom/IEmployee.h>
#include <dom/IService.h>
#include <dom/IProduct.h>

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

	co::TSlice<IEmployee*> getEmployees()
	{
		return _employees;
	}

	void setEmployees( co::Slice<IEmployee*> employees )
	{
		co::assign( employees, _employees );
	}

	co::TSlice<dom::IService*> getServices()
	{
		return _services;
	}

	void setServices( co::Slice<dom::IService*> services )
	{
		co::assign( services, _services );

		for( int i = 0; i < services.getSize(); i++ )
		{
			for( int j = 0; j < services[i]->getMantainers(); j++ )
			{
				_employees.push_back( services[i]->getMantainers()[j] );
			}
			
		}

	}

	co::TSlice<IProduct*> getProducts()
	{
		return _products;
	}

	void setProducts( co::Slice<IProduct*> products )
	{
		co::assign( products, _products );
		for( int i = 0; i < products.getSize(); i++ )
		{
			for( int j = 0; j < products[i]->getDevelopers(); j++ )
			{
				_employees.push_back( products[i]->getDevelopers()[j] );
			}
			_employees.push_back( products[i]->getLeader() );
		}
	}
protected:
	dom::IEmployee* getCeoService()
	{
		return _ceo.get();
	}

	void setCeoService( dom::IEmployee * ceo )
	{
		_ceo = ceo;
	}

private:
	std::vector<IEmployeeRef> _employees;
	std::vector<dom::IServiceRef> _services;
	std::vector<IProductRef> _products;
	IEmployeeRef _ceo;
};

CORAL_EXPORT_COMPONENT( Company, Company )

} // namespace erm
