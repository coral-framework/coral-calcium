local ca = require "ca"
local erm = require "SampleERM"
local dump = require( "lua.pretty" ).dump

function setup()
	erm:setup()
end

function teardown()
	erm:teardown()
end

local function countPairs( t )
	local count = 0
	for _ in pairs( t ) do count = count + 1 end
	return count
end

function universeObserver()
	local universeChanges
	local function universeObserver( changes )
		universeChanges = changes
	end

	ca.observe( erm.universe, universeObserver )
	erm:spaceWithSimpleERM()

	ASSERT_TRUE( universeChanges )
	EXPECT_EQ( erm.universe, universeChanges.graph )
	EXPECT_EQ( 4, #universeChanges.addedObjects )
	EXPECT_EQ( 0, #universeChanges.removedObjects )
	EXPECT_EQ( 0, #universeChanges.changedObjects )

	erm.entityA.name = "A"
	erm.entityB.name = "B"
	erm.entityA.name = "AAA"
	erm.universe:notifyChanges()

	EXPECT_EQ( erm.universe, universeChanges.graph )
	EXPECT_EQ( 0, #universeChanges.addedObjects )
	EXPECT_EQ( 0, #universeChanges.removedObjects )
	EXPECT_EQ( 2, #universeChanges.changedObjects )

	ca.stopObserving( erm.universe, universeObserver )
end

function spaceObserver()
	local spaceChanges
	local function spaceObserver( changes )
		spaceChanges = changes
	end
	ca.observe( erm.space, spaceObserver )

	-- currently spaces are not supported subjects
	-- this shouldn't be hard to implement if proved important
	erm:spaceWithSimpleERM()
	EXPECT_EQ( nil, spaceChanges )

	ca.stopObserving( erm.space, spaceObserver )
end

function objectObserver()
	local objectChanges
	local function objectObserver( changes )
		objectChanges = changes
	end

	erm:spaceWithSimpleERM()

	local relABObj = erm.relAB.provider
	ca.observe( relABObj, objectObserver )

	local gotEvent = {}
	local observeRelABObj = ca.observe( relABObj )
	function observeRelABObj.entityA( change )
		EXPECT_EQ( relABObj, change.object )
		EXPECT_EQ( 'entityA', change.receptacle.name )
		EXPECT_EQ( erm.entityA, change.previous )
		EXPECT_EQ( erm.entityB, change.current )
		gotEvent.entityA = true
	end
	function observeRelABObj.entityB( change )
		EXPECT_EQ( relABObj, change.object )
		EXPECT_EQ( 'entityB', change.receptacle.name )
		EXPECT_EQ( erm.entityB, change.previous )
		EXPECT_EQ( erm.entityA, change.current )
		gotEvent.entityB = true
	end

	relABObj.entityA = erm.entityB
	relABObj.entityB = erm.entityA
	erm.space:notifyChanges()

	ca.stopObserving( relABObj, objectObserver )
	ca.stopObserving( relABObj, observeRelABObj )

	EXPECT_TRUE( gotEvent.entityA )
	EXPECT_TRUE( gotEvent.entityB )

	EXPECT_EQ( relABObj, objectChanges.object )
	EXPECT_EQ( 0, #objectChanges.changedServices )
	EXPECT_EQ( 2, countPairs( objectChanges.changedConnections ) )

	local changedConnection = objectChanges.changedConnections.entityA
	ASSERT_TRUE( changedConnection )
	EXPECT_EQ( relABObj, changedConnection.object )
	EXPECT_EQ( 'entityA', changedConnection.receptacle.name )
	EXPECT_EQ( erm.entityA, changedConnection.previous )
	EXPECT_EQ( erm.entityB, changedConnection.current )

	changedConnection = objectChanges.changedConnections.entityB
	ASSERT_TRUE( changedConnection )
	EXPECT_EQ( relABObj, changedConnection.object )
	EXPECT_EQ( 'entityB', changedConnection.receptacle.name )
	EXPECT_EQ( erm.entityB, changedConnection.previous )
	EXPECT_EQ( erm.entityA, changedConnection.current )
end

function serviceObserver()
	local serviceChanges
	local function serviceObserver( changes )
		serviceChanges = changes
	end

	erm:spaceWithSimpleERM()
	ca.observe( erm.entityA, serviceObserver )

	local gotEvent = false
	local observeEntityA = ca.observe( erm.entityA )
	function observeEntityA.name( change )
		EXPECT_EQ( erm.entityA, change.service )
		EXPECT_EQ( 'name', change.field.name )
		EXPECT_EQ( "Entity A", change.previous )
		EXPECT_EQ( "A", change.current )
		gotEvent = true
	end

	erm.entityA.name = "A"
	erm.space:notifyChanges()

	ca.stopObserving( erm.entityA, serviceObserver )
	ca.stopObserving( erm.entityA, observeEntityA )

	EXPECT_TRUE( gotEvent )

	EXPECT_EQ( erm.entityA, serviceChanges.service )
	EXPECT_EQ( 1, countPairs( serviceChanges.changedFields ) )

	local changedField = serviceChanges.changedFields.name
	EXPECT_EQ( erm.entityA, changedField.service )
	EXPECT_EQ( 'name', changedField.field.name )
	EXPECT_EQ( "Entity A", changedField.previous )
	EXPECT_EQ( "A", changedField.current )
end

function componentObserver()
	local objectChanges = {}
	local function objectObserver( changes )
		objectChanges[#objectChanges + 1] = changes
	end
	ca.observe( "erm.Relationship", objectObserver )

	local connectionChanges = {}
	local observeRelationships = ca.observe( "erm.Relationship" )
	function observeRelationships.entityA( change )
		connectionChanges[#connectionChanges + 1] = change
	end
	function observeRelationships.entityB( change )
		connectionChanges[#connectionChanges + 1] = change
	end

	erm:spaceWithExtendedERM()

	EXPECT_EQ( 0, #objectChanges )
	EXPECT_EQ( 0, #connectionChanges )

	erm.relAB.provider.entityA = erm.entityB
	erm.relAB.provider.entityB = erm.entityC
	erm.relBC.provider.entityA = erm.entityC
	erm.relBC.provider.entityB = erm.entityA
	erm.relCA.provider.entityA = erm.entityA
	erm.relCA.provider.entityB = erm.entityB
	erm.space:notifyChanges()

	ASSERT_EQ( 3, #objectChanges )
	EXPECT_EQ( 2, countPairs( objectChanges[1].changedConnections ) )
	EXPECT_EQ( 2, countPairs( objectChanges[2].changedConnections ) )
	EXPECT_EQ( 2, countPairs( objectChanges[3].changedConnections ) )
	ASSERT_EQ( 6, #connectionChanges )

	ca.stopObserving( "erm.Relationship", objectObserver )
	ca.stopObserving( "erm.Relationship", observeRelationships )
end

function interfaceObserver()
	local serviceChanges = {}
	local function serviceObserver( changes )
		serviceChanges[#serviceChanges + 1] = changes
	end
	ca.observe( "erm.IEntity", serviceObserver )

	local fieldChanges = {}
	local observeIEntities = ca.observe( "erm.IEntity" )
	function observeIEntities.name( change )
		fieldChanges[#fieldChanges + 1] = change
	end

	erm:spaceWithExtendedERM()

	EXPECT_EQ( 0, #serviceChanges )
	EXPECT_EQ( 0, #fieldChanges )

	erm.entityA.name = "AA"
	erm.entityB.name = "BB"
	erm.entityC.name = "CC"
	erm.space:notifyChanges()

	ASSERT_EQ( 3, #serviceChanges )
	ASSERT_EQ( 3, #fieldChanges )

	ca.stopObserving( "erm.IEntity", serviceObserver )
	ca.stopObserving( "erm.IEntity", observeIEntities )
end

function arraySpecialFields()
	local fieldChanges = {}
	local observeIModel = ca.observe "erm.IModel"
	function observeIModel.entities( change )
		fieldChanges[#fieldChanges + 1] = change
	end

	erm:spaceWithExtendedERM()

	erm.erm:addEntity( erm.entityA )
	erm.erm:addEntity( erm.entityB )
	erm.erm:addEntity( erm.entityB )
	erm.space:notifyChanges()

	ASSERT_EQ( 1, #fieldChanges )

	local change = fieldChanges[1]
	EXPECT_EQ( 3, #change.previous )
	EXPECT_EQ( 6, #change.current )
	EXPECT_EQ( 2, countPairs( change.added ) )
	EXPECT_EQ( 1, change.added[erm.entityA] )
	EXPECT_EQ( 2, change.added[erm.entityB] )
	EXPECT_EQ( 0, countPairs( change.removed ) )

	erm.erm:removeEntity( erm.entityB )
	erm.erm:removeEntity( erm.entityC )
	erm.erm:addEntity( erm.entityA )
	erm.erm:addEntity( erm.entityB )
	erm.space:notifyChanges()

	ASSERT_EQ( 2, #fieldChanges )

	local change = fieldChanges[2]
	EXPECT_EQ( 6, #change.previous )
	EXPECT_EQ( 4, #change.current )
	EXPECT_EQ( 1, countPairs( change.added ) )
	EXPECT_EQ( 1, change.added[erm.entityA] )
	EXPECT_EQ( 2, countPairs( change.removed ) )
	EXPECT_EQ( 2, change.removed[erm.entityB] )
	EXPECT_EQ( 1, change.removed[erm.entityC] )

	ca.stopObserving( "erm.IModel", observeIModel )
end

function misusingTheAPI()
	local nativeClassInstance = co.new "erm.Multiplicity"
	local function closure() end

	-- tests for ca.observe()

	EXPECT_ERROR( "illegal observer %(expected a function, got a table%)",
		function() ca.observe( "erm.Entity", {} ) end )

	EXPECT_ERROR( "illegal subject instance %(expected an object or service, got an instance of erm.Multiplicity%)", function() ca.observe( nativeClassInstance, closure ) end )

	EXPECT_ERROR( "type 'erm.NonExisting' was not found in the path",
		function() ca.observe( "erm.NonExisting", closure ) end )

	EXPECT_ERROR( "illegal subject type %(erm.Multiplicity is neither an interface nor a component%)",
		function() ca.observe( "erm.Multiplicity", closure ) end )

	EXPECT_ERROR( "illegal subject %(expected type name or service, got table%)",
		function() ca.observe( {}, closure ) end )

	ca.observe( "erm.Entity", closure )
	EXPECT_ERROR( "registering the same observer twice",
		function() ca.observe( "erm.Entity", closure ) end )

	-- tests for ca.stopObserving()

	EXPECT_ERROR( "could not load type 'erm.NonExisting'",
		function() ca.stopObserving( "erm.NonExisting", closure ) end )

	ca.stopObserving( "erm.Entity", closure )

	-- tests for field observer tables

	local observeIEntity = ca.observe "erm.IEntity"
	EXPECT_ERROR( "observer tables can only have strings as keys",
		function() observeIEntity[3] = closure end )

	EXPECT_ERROR( "observer tables can only have functions as values",
		function() observeIEntity.name = 3 end )

	EXPECT_ERROR( "cannot observe non%-existing member 'nonExisting' in subject of type erm.IEntity",
		function() observeIEntity.nonExisting = closure end )

	ca.stopObserving( "erm.IEntity", observeIEntity )
	EXPECT_ERROR( "cannot modify an unregistered observer table",
		function() observeIEntity.name = closure end )
end
