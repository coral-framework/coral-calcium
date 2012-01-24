#ifndef _SPACE_UPDATER_
#define _SPACE_UPDATER_

#include <ca/ISpaceStore.h>
#include <ca/IModel.h>

#include <set>

namespace ca {

	class SpaceUpdater
	{
	public:

		SpaceUpdater( ca::ISpaceStore* store )
		{
			_store = store;
		}

		~SpaceUpdater() {;}

		bool checkModelChange( ca::IModel* model, ca::StoredType storedType );

		void update();
	private:
		bool evaluateChanges( ca::IModel* model, ca::StoredType storedType );
	private:
		ca::ISpaceStore* _store;
		std::set<co::IType*> _notChangedTypes;

	};

} // namespace ca

#endif