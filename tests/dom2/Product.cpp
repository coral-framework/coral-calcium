#include "Product_Base.h"
#include <dom/IProduct.h>
#include <dom/IEmployee.h>
#include <co/RefVector.h>
#include <co/RefPtr.h>

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

		const std::string& getName() { return _name; }
		void setName( const std::string& name ) { _name = name; }

		double getValue() { return _earnings; }
		void setValue( double earnings ) { _earnings = earnings; }

		co::Range<IEmployee* const> getDevelopers()
		{
			return _developers;
		}

		void setDevelopers( co::Range<IEmployee* const> developers )
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
		co::RefVector<IEmployee> _developers;
		co::RefPtr<IEmployee> _leader;

	};

	CORAL_EXPORT_COMPONENT( Product, Product )

} // namespace erm
