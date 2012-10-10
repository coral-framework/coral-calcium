#include "Manager_Base.h"
#include <dom/IProject.h>
#include <dom/IDeveloper.h>
#include <co/RefVector.h>

namespace dom {

	class Manager : public Manager_Base
	{
	public:
		Manager() : _project( NULL )
		{
			// empty
		}

		virtual ~Manager()
		{
			// empty
		}

		IProject* getProject() { return _project; }
		void setProject( IProject* project ) { _project = project; }

		const std::string& getName() { return _name; }
		void setName( const std::string& name ) { _name = name; }

		double getSalary() { return _salary; }
		void setSalary( double salary ) { _salary = salary; }

		co::Range<IDeveloper*> getWorkers()
		{
			return _workers;
		}

		void setWorkers( co::Range<IDeveloper*> workers )
		{
			co::assign( workers, _workers );
		}

	protected:


	private:
		IProject* _project;
		std::string _name;
		double _salary;
		co::RefVector<IDeveloper> _workers;

	};

	CORAL_EXPORT_COMPONENT( Manager, Manager )

} // namespace erm
