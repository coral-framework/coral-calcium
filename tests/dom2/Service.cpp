#include "Service_Base.h"
#include <dom/IService.h>
#include <dom/IEmployee.h>
#include <co/RefVector.h>
#include <co/RefPtr.h>

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

		const std::string& getName() { return _name; }
		void setName( const std::string& name ) { _name = name; }

		double getMonthlyIncome() { return _earnings; }
		void setMonthlyIncome( double earnings ) { _earnings = earnings; }

		co::Range<IEmployee* const> getMantainers()
		{
			return _developers;
		}

		void setMantainers( co::Range<IEmployee* const> developers )
		{
			co::assign( developers, _developers );
		}

	protected:


	private:
		std::string _name;
		double _earnings;
		co::RefVector<IEmployee> _developers;

	};

	CORAL_EXPORT_COMPONENT( Service, Service )

} // namespace erm
