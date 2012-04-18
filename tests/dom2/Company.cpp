#include "Company_Base.h"
#include <dom/IEmployee.h>
#include <dom/IService.h>
#include <dom/IProduct.h>
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

		co::Range<IEmployee* const> getEmployees()
		{
			return _employees;
		}

		void setEmployees( co::Range<IEmployee* const> employees )
		{
			co::assign( employees, _employees );
		}

		co::Range<dom::IService* const> getServices()
		{
			return _services;
		}

		void setServices( co::Range<dom::IService* const> services )
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

		co::Range<IProduct* const> getProducts()
		{
			return _products;
		}

		void setProducts( co::Range<IProduct* const> products )
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

	private:
		co::RefVector<IEmployee> _employees;
		co::RefVector<dom::IService> _services;
		co::RefVector<IProduct> _products;
	};

	CORAL_EXPORT_COMPONENT( Company, Company )

} // namespace erm
