#include "Employee_Base.h"
#include <dom/IProduct.h>
#include <dom/IService.h>

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

		const std::string& getRole()
		{
			return _role;
		}

		void setRole( const std::string& role )
		{
			_role = role;
		}

		dom::IService* getService()
		{
			return _service;
		}

		void setService( dom::IService* service )
		{
			_service = service;
		}

		IProduct* getProduct() { return _product; }
		void setProduct( IProduct* project ) { _product = _product; }

		const std::string& getName() { return _name; }
		void setName( const std::string& name ) { _name = name; }

		double getSalary() { return _salary; }
		void setSalary( double salary ) { _salary = salary; }

	protected:


	private:
		IProduct* _product;
		dom::IService* _service;
		std::string _name;
		std::string _role;
		double _salary;

	};

	CORAL_EXPORT_COMPONENT( Employee, Employee )

} // namespace erm
