/*
 * Calcium - Domain Model Framework
 * See copyright notice in LICENSE.md
 */

#include <gtest/gtest.h>

#include <co/Coral.h>
#include <co/Platform.h>
#include <co/IObject.h>
#include <ca/IUniverse.h>

TEST( UniverseTests, setup )
{
	//co::RefPtr<co::IObject> universeObj = co::newInstance( "ca.Universe" );
	//ca::IUniverse* universe = universeObj->getService<ca::IUniverse>();

	// should not be able to use the universe before its 'model' is set
	//EXPECT_THROW( universe->begin, <#expected_exception#>)

	//co::bindService( universeObj.get(), "model", model );
}
