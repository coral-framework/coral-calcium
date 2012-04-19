--------------------------------------------------------------------------------
-- Initialization
--------------------------------------------------------------------------------

local type = type
local next = next
local pairs = pairs
local coLog = co.log
local assert = assert
local xpcall = xpcall
local rawset = rawset
local coType = co.Type
local coTypeOf = co.typeOf
local tostring = tostring
local setmetatable = setmetatable
local dump = require( "lua.pretty" ).dump
local traceback = require( "debug" ).traceback

co.system.modules:load( "ca" )

--------------------------------------------------------------------------------
-- Smart Change Events (with memoized, simplified and lazily computed fields)
--------------------------------------------------------------------------------

local function createEventMT( fieldCalculators )
	return { __index = function( t, k )
		local f = fieldCalculators[k]
		if f then
			local v = f( t, k )
			t[k] = v
			return v
		end
	end }
end

local function unchanged( t, k )
	return t._ref[k]
end

local function makeArraySmart( array, MT )
	for i = 1, #array do
		array[i] = setmetatable( { _ref = array[i] }, MT )
	end
	return array
end

local function arrayToSmartMap( array, map, indexFieldName, MT )
	for i = 1, #array do
		local v = setmetatable( { _ref = array[i] }, MT )
		map[v[indexFieldName].name] = v
	end
	return map
end

local fieldEventMT = createEventMT{
	field = unchanged,
	previous = unchanged,
	current = unchanged,
	service = function( t )
		local serviceEvent = t._sup
		if serviceEvent then
			return serviceEvent.service
		end
	end,
	-- special fields for detecting changes in arrays:
	countOf = function( t )
		-- INTERNAL: a set { value => #occurrences } based on 'previous'
		local previous = t.previous
		assert( type( previous ) == 'table', "field is not an array" )
		local set = {}
		for i = 1, #previous do
			local v = previous[i]
			set[v] = ( set[v] or 0 ) + 1
		end
		return set
	end,
	added = function( t )
		-- a map { value => #added } for the values in 'current' which are not in 'previous'
		local current, countOf = t.current, t.countOf
		local res = {}
		for i = 1, #current do
			local v = current[i]
			local count = countOf[v]
			if not count or count <= 0 then
				res[v] = ( res[v] or 0 ) + 1
			else
				countOf[v] = count - 1
			end
		end
		return res
	end,
	removed = function( t )
		-- a map { value => #removed } for the values in 'previous' removed from 'current'
		local added = t.added -- triggers required calculations
		local res = {}
		for v, count in pairs( t.countOf ) do
			if count > 0 then
				res[v] = count
			end
		end
		return res
	end
}

local serviceEventMT = createEventMT{
	service = unchanged,
	changedFields = function( t, k )
		local map = {}
		arrayToSmartMap( t._ref.changedRefFields, map, 'field', fieldEventMT )
		arrayToSmartMap( t._ref.changedRefVecFields, map, 'field', fieldEventMT )
		arrayToSmartMap( t._ref.changedValueFields, map, 'field', fieldEventMT )
		return map
	end,
}

local connectionEventMT = createEventMT{
	receptacle = unchanged,
	previous = unchanged,
	current = unchanged,
	object = function( t )
		local objectEvent = t._sup
		if objectEvent then return objectEvent.object end
	end
}

local objectEventMT = createEventMT{
	object = unchanged,
	changedServices = function( t, k ) return makeArraySmart( t._ref[k], serviceEventMT ) end,
	changedConnections = function( t, k )
		return arrayToSmartMap( t._ref[k], {}, 'receptacle', connectionEventMT )
	end,
	changedFields = function( t ) return t.changedConnections end
}

local graphEventMT = createEventMT{
	graph = unchanged,
	addedObjects = unchanged,
	removedObjects = unchanged,
	changedObjects = function( t, k ) return makeArraySmart( t._ref[k], objectEventMT ) end
}

--------------------------------------------------------------------------------
-- Change Event Dispatchers
--------------------------------------------------------------------------------

--[[
 This table maps { subject => { observer => kind } }. An 'observer' can be either a closure
 or a table of closures. If it is a closure, it's called directly with a subject change event
 table. Otherwise, if it's a table of closures, only the closures whose keys correspond to
 a changed 'field name' are called (in this case, with a 'field' event table).
--]]
local observers = {}

local function sendEvent( closure, event )
	local ok, err = xpcall( closure, traceback, event )
	if not ok then
		coLog( 'ERROR', "Calcium caught an error while notifying an observer: " .. dump( err ) )
	end
end

local function dispatchToMap( event, observerMap )
	for observer, kind in pairs( observerMap ) do
		if kind == 'function' then
			sendEvent( observer, event )
		else -- observer is a table { 'field name' => closure }
			local changedFields = event.changedFields
			for fieldName, fieldObserver in pairs( observer ) do
				local fieldEvent = changedFields[fieldName]
				if fieldEvent then
					fieldEvent._sup = event -- enables access to the 'parent' event
					sendEvent( fieldObserver, fieldEvent )
				end
			end
		end
	end
end

local function dispatch( event, subject, defaultHandler )
	local instanceObservers = observers[subject]
	if instanceObservers then
		dispatchToMap( event, instanceObservers )
	end
	local typeObservers = observers[coTypeOf( subject )]
	if typeObservers then
		dispatchToMap( event, typeObservers )
	end
	if defaultHandler then
		defaultHandler( event )
	end
end

local function dispatchChangedServices( objectEvent )
	local changedServices = objectEvent.changedServices
	for i = 1, #changedServices do
		local serviceEvent = changedServices[i]
		dispatch( serviceEvent, serviceEvent.service )
	end
end

local function dispatchChangedObjects( graphEvent )
	local changedObjects = graphEvent.changedObjects
	for i = 1, #changedObjects do
		local objectEvent = changedObjects[i]
		dispatch( objectEvent, objectEvent.object, dispatchChangedServices )
	end
end

--------------------------------------------------------------------------------
-- Observers Management
--------------------------------------------------------------------------------

-- maps observer tables to their subjects
local observerTables = {}

local fieldObserversMT = {
	__newindex = function( t, key, value )
		if type( key ) ~= 'string' then error( "observer tables can only have strings as keys", 2 ) end
		if type( value ) ~= 'function' then error( "observer tables can only have functions as values", 2 ) end
		local subject = observerTables[t]
		if not subject then error( "cannot modify an unregistered observer table", 2 ) end
		local typeName = ( type( subject ) == 'string' and subject or coTypeOf( subject ) )
		if not coType[typeName]:getMember( key ) then
			error( "cannot observe non-existing member '" .. key .. "' in subject of type " .. typeName, 2 )
		end
		rawset( t, key, value )
	end
}

local function registerObserver( subject, observer, observerType )
	local subjectObservers = observers[subject]
	if not subjectObservers then
		subjectObservers = {}
		observers[subject] = subjectObservers
	end
	assert( subjectObservers[observer] == nil, "registering the same observer twice?" )
	observerType = observerType or type( observer )
	assert( observerType == 'function' or observerType == 'table', "unsupported observer type" )
	subjectObservers[observer] = observerType
end

local function unregisterObserver( subject, observer )
	local subjectObservers = observers[subject]
	if not subjectObservers then
		coLog( 'WARNING', "ca.stopObserving(): subject '" .. tostring( subject ) .. "' is not being observed" )
		return false
	end
	if not subjectObservers[observer] then
		coLog( 'WARNING', "ca.stopObserving(): no such observer '"  .. tostring( observer ) ..
			"' for subject '" .. tostring( subject ) .. "'" )
	end
	subjectObservers[observer] = nil
	-- if this was the last observer for the subject, remove the table
	if not next( subjectObservers ) then
		observers[subject] = nil
	end
	return true
end

local function registerInterfaceObserver( itfType, observer, observerType )
	registerObserver( itfType.fullName, observer, observerType )
	-- also register the observer for all known subtypes of the interface
	local subTypes = itfType.subTypes
	for i = 1, #subTypes do
		registerInterfaceObserver( subTypes[i], observer, observerType )
	end
end

local function unregisterInterfaceObserver( itfType, observer )
	local res = unregisterObserver( itfType.fullName, observer )
	-- also unregister the observer for all known subtypes of the interface
	local subTypes = itfType.subTypes
	for i = 1, #subTypes do
		local succeeded = unregisterInterfaceObserver( subTypes[i], observer )
		res = res and succeeded
	end
	return res
end

--------------------------------------------------------------------------------
-- Universal Graph Observer Component
--------------------------------------------------------------------------------

local UniversalObserver = co.Component {
	name = "ca.UniversalObserver",
	provides = { graphObserver = "ca.IGraphObserver" }
}

function UniversalObserver:onGraphChanged( changes )
	local graphEvent = setmetatable( { _ref = changes }, graphEventMT )
	dispatch( graphEvent, graphEvent.graph, dispatchChangedObjects )
end

function UniversalObserver:__gc()
	observers = nil
	observerTables = nil
end

co.getService( "ca.ILuaManager" ).observer = UniversalObserver().graphObserver

--------------------------------------------------------------------------------
-- Module Functions
--------------------------------------------------------------------------------

local M = {}

--[[
 Registers a change observer for a subject.

 A subject is either an instance of a service/object/graph, or the full name of
 an interface/component whose instances should be observed.

 This function can be called in two ways: if a closure is passed as the second
 argument, it's directly registered as a general observer for the subject.
 Otherwise, if the second argument is omitted, the function returns a special
 table where you can register individual field observers for the subject.

 Examples:
 	-- Print field value changes in all services of type 'dom.IEntity'
	ca.observe( "dom.IEntity", function( e )
		print( "Entity " .. tostring( e.service ) .. " changed:" )
		for fieldName, change in pairs( e.changedFields ) do
			print( "  " .. fieldName .. " is now " .. tostring( change.current ) )
		end
	end )

	-- Observe changes to specific fields using a closure for each field
	local observeEntity = ca.observe( "dom.IEntity" )
	function observeEntity.name( e )
		print( "Entity " .. tostring( e.service ) ..
			" renamed from " .. e.previous .. " to " .. e.current )
	end
	function observeEntity.position( e )
		print( "Entity " .. tostring( e.service ) ..
			" moved from " .. e.previous .. " to " .. e.current )
	end
--]]
function M.observe( subject, observer )
	-- validate the observer
	local observerType
	if observer then
		observerType = type( observer )
		if observerType ~= 'function' then
			error( "illegal observer (expected a function, got a " .. observerType .. ")", 2 )
		end
	else
		observerType = 'table'
		observer = setmetatable( {}, fieldObserversMT )
	end
	-- validate the subject
	local subjectType = type( subject )
	if subjectType == 'userdata' then
		local typeName = coTypeOf( subject )
		if not typeName then error( "unknown instance type", 2 ) end
		local kind = coType[typeName].kind
		if kind ~= 'TK_INTERFACE' and kind ~= 'TK_COMPONENT' then
			error( "illegal subject instance (expected an object or service, got an instance of "
				.. typeName .. ")", 2 )
		end
		registerObserver( subject, observer, observerType )
	elseif subjectType == 'string' then
		local coralType = coType[subject]
		local kind = coralType.kind
		if kind == 'TK_INTERFACE' then
			registerInterfaceObserver( coralType, observer, observerType )
		elseif kind == 'TK_COMPONENT' then
			registerObserver( subject, observer, observerType )
		else
			error( "illegal subject type (" .. subject .. " is neither an interface nor a component)", 2 )
		end
	else
		error( "illegal subject (expected type name or service, got " .. subjectType .. ")",  2 )
	end
	if observerType == 'table' then
		observerTables[observer] = subject
	end
	return observer
end

--[[
 Unregisters a change observer for a subject.
 Returns a boolean indicating whether the observer was successfully removed.
--]]
function M.stopObserving( subject, observer )
	if observerTables[observer] then
		observerTables[observer] = nil
	end
	if type( subject ) == 'string' then
		local coralType = coType[subject]
		if coralType.kind == 'TK_INTERFACE' then
			local res = unregisterInterfaceObserver( coralType, observer )
			if not res then
				coLog( 'ERROR', "ca.stopObserving(): not all subtypes of " .. subject
					.. " had been registered for observer '" .. tostring( observer )
					.. "'; it may have missed notifications" )
			end
			return res
		end
	end
	return unregisterObserver( subject, observer )
end

return M
