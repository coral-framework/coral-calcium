#include "Project_Base.h"
#include <dom/IProject.h>
#include <dom/IManager.h>
#include <dom/IDeveloper.h>
#include <co/RefVector.h>
#include <co/RefPtr.h>

namespace dom {

	class Project : public Project_Base
	{
	public:
		Project()
		{
			// empty
		}

		virtual ~Project()
		{
			// empty
		}

		const std::string& getName() { return _name; }
		void setName( const std::string& name ) { _name = name; }

		double getEarnings() { return _earnings; }
		void setEarnings( double earnings ) { _earnings = earnings; }

		co::Range<IDeveloper* const> getDevelopers()
		{
			return _developers;
		}

		void setDevelopers( co::Range<IDeveloper* const> developers )
		{
			co::assign( developers, _developers );
		}

		bool getIsService()
		{
			return _isService;
		}

		void setIsService( bool isService )
		{
			_isService = isService;
		}

		IManager* getManager()
		{
			return _manager.get();
		}

		void setManager( dom::IManager* manager )
		{
			_manager = manager;
		}

	protected:


	private:
		std::string _name;
		double _earnings;
		bool _isService;
		co::RefVector<IDeveloper> _developers;
		co::RefPtr<IManager> _manager;

	};

	CORAL_EXPORT_COMPONENT( Project, Project )

} // namespace erm
