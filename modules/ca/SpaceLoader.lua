local updateEnv = {}

updateEnv.index = {}
updateEnv.assignmentCache = {}
local assignmentCache = updateEnv.assignmentCache

-- create metatable
updateEnv.updateMT = {
	__index = function (t,k)
		local returnValue = t[updateEnv.index][k]
		return returnValue
	end,
	__newindex = function (t,k,v)

		if t[updateEnv.index] ~= nil and t[updateEnv.index]["_id"] ~= nil then
			if assignmentCache[t] == nil then
				assignmentCache[t] = {}
			end
			assignmentCache[t][k] = v
		end
		if type( v ) == 'table' and v._facet then
			v._provider = t
			v._facet = nil
		end
		
		t[updateEnv.index][k] = v   -- update original table
	end
}

function updateEnv.track (t)
  local proxy = {}
  proxy[updateEnv.index] = t
  setmetatable( proxy, updateEnv.updateMT )
  return proxy
end
local track = updateEnv.track


function updateEnv.newInstance( typeStr )

	local cotype, err = co.Type( typeStr )
	if err then
		error( err )
	end
	local ports = updateEnv.model:getPorts( cotype )
	local t = { _type = typeStr }
	t = track(t)
	for _, port in ipairs( ports ) do
		if port.isFacet then
			local facetTable = updateEnv.Facet( port.type.fullName )
			t[ port.name ] = facetTable
			facetTable._provider = t
		end
	end
	return t
end 

updateEnv.facetMT = { __call = function( facetTable, initialValues )
	if initialValues ~= nil and type( initialValues ) == 'table' then
		for k, v in pairs( initialValues ) do
			facetTable[k] = v
		end
	end
	return track( facetTable )
end }

function updateEnv.Facet( serviceType )
	local facetTable = setmetatable( { _type = serviceType, _facet = true }, updateEnv.facetMT )
	
	return facetTable
end

local updateEnvMT = { __index = function( t, k )
							local returnValue = updateEnv[k]
							
							if returnValue == nil then
								returnValue = _G[k]
							end
							return returnValue
						end
					}

local newEnv = { updateEnvMT = updateEnvMT } -- create new environment
setmetatable( newEnv, {__index = _G} )
_ENV = newEnv
						
local function loadFileIn( filePath, env )
	local file, err = io.open( filePath, 'rb' )
	if file then
		local chunk, err = load( file:lines( 4096 ), filePath, 't', env )
		file:close()
		return chunk, err
    end
    return file, err
end

local idCache = {}
local conversionCache = {}

local namespaces = {}

local coNew = co.new
local coType = co.Type
local coRaise = co.raise

function extractNamespaceFullName( typeName )
	for ns in typeName:gmatch( "([%w%.]+)%.%w+" ) do
		return ns
	end
end

function refKind( fieldValue )

	if fieldValue:sub(1,1) == '#' then
		if fieldValue:sub(2,2) == '{' then
			--refvec
			return 2;
		end
		return 1;
	end
	return 0

end

function restoreService( spaceStore, objModel, objectId, serviceId, revision )
	if idCache[objectId] == nil then
		restoreObject( spaceStore, objModel, objectId, revision )
	elseif idCache[serviceId] == nil then

		local typeName = spaceStore:getObjectType( serviceId, revision )

		namespaces[ extractNamespaceFullName( typeName ) ] = true

		local luaObjectTable = { _type = typeName, _id = serviceId }
		idCache[ serviceId ] = track( luaObjectTable )

		local fieldNames = {}
		local values = {}
		fieldNames, values = spaceStore:getValues( serviceId, revision )

		for i, value in ipairs( values ) do
			local refKind = refKind( value )
			if  refKind == 1 then
				local idServiceStr = value:sub( 2 )
				local chunk = load( "return " .. idServiceStr )
				local idServiceFieldValue = chunk()

				if idServiceFieldValue == nil then
					luaObjectTable[ fieldNames[i] ] = nil
				else
					local objProvider = spaceStore:getServiceProvider( idServiceFieldValue, revision )

					restoreService( spaceStore, objModel, objProvider, idServiceFieldValue, revision )

					luaObjectTable[ fieldNames[i] ] = idCache[ idServiceFieldValue ]
				end
			elseif refKind == 2 then
				local idServiceListStr = value:sub( 2 )

				local chunk = load( "return " .. idServiceListStr )
				local idServiceList = chunk()

				local serviceList = {}

				for i, idService in ipairs( idServiceList ) do
					local objProvider = spaceStore:getServiceProvider( idService, revision )

					restoreService( spaceStore, objModel, objProvider, idService, revision )

					serviceList[ #serviceList + 1 ] = idCache[ idService ]

				end

				luaObjectTable[ fieldNames[i] ] = serviceList

			else
				local chunk = load( "return " .. value )
				luaObjectTable[ fieldNames[i] ] = chunk()
			end
		end
		luaObjectTable._provider = idCache[ objectId ]
	end

	return idCache[ serviceId ]
end

function restoreObject( spaceStore, objModel, objectId, revision )
	if idCache[objectId] == nil then

		local typeName = spaceStore:getObjectType( objectId, revision )
		local luaObjectTable = { _type = typeName, _id = objectId }
		idCache[ objectId ] = track( luaObjectTable )

		local fieldNames = {}
		local values = {}
		fieldNames, values = spaceStore:getValues( objectId, revision )

		for i, value in ipairs( values ) do
			serviceIdStr = value:sub(2)
			local chunk = load( "return " .. serviceIdStr )
			local serviceId = chunk()
			local service = restoreService( spaceStore, objModel, objectId, serviceId, revision )

			luaObjectTable[ fieldNames[i] ] = service
		end

	end
	return idCache[ objectId ]
end

function restore( space, spaceStore, objModel, revision, spaceLoader )
	updateEnv.model = objModel
	spaceStore:open()

	local rootId = spaceStore:getRootObject( revision )

	local obj = restoreObject( spaceStore, objModel, rootId, revision )

	local appliedUpdates = spaceStore:getUpdates( revision )

	loadCaModels( objModel )
	local availableUpdates = objModel.updates

	local hasApplied = {}

	for script in appliedUpdates:gmatch( "[^;]+" ) do
	   hasApplied[script] = true
	end

	for _, script in ipairs( availableUpdates ) do
		if not hasApplied[script] then
			applyUpdate( script, obj )
			appliedUpdates = appliedUpdates .. script ..";"
		end
	end

	spaceStore:close()
	coralObj = convertToCoral( obj, objModel, spaceLoader )
	space:initialize( coralObj )
	space:notifyChanges()

	spaceLoader:setUpdateList( appliedUpdates )
end

function loadCaModels( objModel )
	for ns, _ in pairs( namespaces ) do
		objModel:loadDefinitionsFor( ns )
	end

end

function applyUpdate( updateScript, coralGraph )
	local path = co.findScript( updateScript )

	if path == nil then
		coRaise( "ca.IOException", "Script file not found" )
	end

	currentEnv = setmetatable( {}, updateEnvMT )
	updateList = {}

	local chunk, err = loadFileIn( path, currentEnv )
		
	if ( chunk ) then
		chunk()
		local ok, result = pcall( currentEnv.update, coralGraph )
		-- unload the chunk and update function
		
		chunk = nil
		update = nil

		if not ok then
			coRaise( "ca.IOException", result )
		end
		return result
	else
		coRaise( "ca.IOException", err )
	end
    
	return nil

end

function convertToCoral( obj, objModel, spaceLoader )

	if conversionCache[obj] == nil then
		local root = coNew( obj._type )

		local currentService

		conversionCache[ obj ] = root

		if obj._id == nil then
			spaceLoader:insertNewObject( root )
		else
			spaceLoader:insertObjectCache( root, obj._id )
		end

		if ( assignmentCache[ obj ] ~= nil ) and ( assignmentCache[ obj ]["_type"] ~= nil ) then
			spaceLoader:addTypeChange( root, obj._type )
		end

		local ports = objModel:getPorts( root.component )
		local portName
		for i, port in ipairs( ports ) do
			portName = port.name
			currentService = root[portName]
			currentServiceValues = obj[portName]
			
			if port.isFacet then
				fillServiceValues( currentService, currentServiceValues, objModel, spaceLoader )
			else
				if currentServiceValues ~= nil and conversionCache[ currentServiceValues ] == nil then
					local providerObj = convertToCoral( currentServiceValues._provider, objModel, spaceLoader )
					currentService = conversionCache[ currentServiceValues ]
					root[portName] = currentService
				end
			end
			if obj._id ~= nil and assignmentCache[ obj ] ~= nil and assignmentCache[ obj ][ portName ] ~= nil then
				spaceLoader:addRefChange( root, port, currentService )
			end
		end
	end
	return conversionCache[ obj ]
end

function fillServiceValues( service, serviceValues, objModel, spaceLoader )
	conversionCache[serviceValues] = service
	if serviceValues._id == nil then
		if serviceValues._provider._id ~= nil then 
			spaceLoader:insertNewObject( service )
		end
	else
		spaceLoader:insertObjectCache( service, serviceValues._id )
	end

	local fields = objModel:getFields( service.interface )

	local fieldKind
	local fieldValue
	local fieldName

	local hasChange
	local refVec

	if ( assignmentCache[ serviceValues ] ~= nil ) and ( assignmentCache[ serviceValues ]["_type"] ~= nil ) then
		spaceLoader:addTypeChange( service, service.interface.fullName )
	end

	for i, field in ipairs( fields ) do
		fieldKind = field.type.kind
		fieldName = field.name
		fieldValue = serviceValues[fieldName]
		if fieldValue ~= nil then
			hasChange = (assignmentCache[ serviceValues ] ~= nil) and (assignmentCache[ serviceValues ][fieldName] ~= nil)
			refVec = (fieldKind == 'TK_ARRAY') and (field.type.elementType.kind == 'TK_INTERFACE' )
			if refVec then
				local serviceArray = {}
				for _, svInd in ipairs( fieldValue ) do
					convertToCoral( svInd._provider, objModel, spaceLoader )
					serviceArray[ #serviceArray + 1 ] = conversionCache[ svInd ]
				end
				service[fieldName] = serviceArray
				if hasChange then
					spaceLoader:addRefVecChange( service, field, service[fieldName] )
				end
			elseif fieldKind == 'TK_INTERFACE' then
				local objProvider = convertToCoral( fieldValue._provider, objModel, spaceLoader )
				service[fieldName] = conversionCache[fieldValue]
				if hasChange then
					spaceLoader:addRefChange( service, field, service[fieldName] )
				end
			elseif fieldKind == 'TK_STRUCT' then
				local struct = service[fieldName]
				for k, v in pairs( fieldValue ) do
					struct[k] = v
				end
				service[fieldName] = struct
				if hasChange then
					spaceLoader:addChange( service, field, service[fieldName] )
				end
			else
				service[fieldName] = fieldValue
				if hasChange then
					spaceLoader:addChange( service, field, service[fieldName] )
				end
			end
		end
	end
end

local function protectedRestoreSpace( space, spaceStore, objModel, revision, spaceLoader )
	idCache = {}
	coralCache = {}

	local ok, result = pcall( restore, space, spaceStore, objModel, revision, spaceLoader )
	if not ok then
		print( result )
		coRaise( "ca.IOException", result )
	end
end

return protectedRestoreSpace
