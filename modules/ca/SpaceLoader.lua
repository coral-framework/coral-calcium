local idCache = {}
local coralCache = {}

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
	if idCache[serviceId] == nil then
	
		local typeName
		
		local typeName = spaceStore:getObjectType( serviceId )
		
		local luaObjectTable = { _type = typeName, _id = serviceId }
		idCache[ serviceId ] = luaObjectTable
		
		local values = {}
		values = spaceStore:getValues( serviceId, revision )
		
		for i, value in ipairs( values ) do
			if  refKind( value.value ) == 1 then
				local idServiceStr = value.value:sub( 2 )
				local chunk = load( "return " .. idServiceStr )
				local idServiceFieldValue = chunk()
				
				if idServiceFieldValue == nil then
					luaObjectTable[ value.fieldName ] = nil			
				else
					local objProvider = spaceStore:getServiceProvider( idServiceFieldValue )
					
					restoreObject( spaceStore, objModel, objProvider, revision )
					
					luaObjectTable[ value.fieldName ] = idCache[ idServiceFieldValue ]
				end
			elseif refKind( value.value ) == 2 then
				local idServiceListStr = value.value:sub( 2 )
				local chunk = load( "return " .. idServiceListStr )
				local idServiceList = chunk()
				
				local serviceList = {}
				
				for i, idService in ipairs( idServiceList ) do
					
					local objProvider = spaceStore:getServiceProvider( idService )
				
					restoreObject( spaceStore, objModel, objProvider, revision )
					
					serviceList[ #serviceList + 1 ] = idCache[ idService ]
				end
				
				luaObjectTable[ value.fieldName ] = serviceList
				
			else
				local chunk = load( "return " .. value.value )
				luaObjectTable[ value.fieldName ] = chunk()
			end
		end
		luaObjectTable._provider = objectId
	end
	
	return idCache[ serviceId ]
end

function restoreObject( spaceStore, objModel, objectId, revision )
	if idCache[objectId] == nil then
		local typeName
		
		local typeName = spaceStore:getObjectType( objectId )
		
		local luaObjectTable = { _type = typeName, _id = objectId }
		idCache[ objectId ] = luaObjectTable
		
		objModel:contains( co.getType( typeName ) )
		
		local values = {}
		values = spaceStore:getValues( objectId, revision )
		
		for i, value in ipairs( values ) do
			serviceIdStr = value.value:sub(2)
			local chunk = load( "return " .. serviceIdStr )
			local serviceId = chunk()
			local service = restoreService( spaceStore, objModel, objectId, serviceId, revision )
			
			luaObjectTable[ value.fieldName ] = service
		end
		
	end
	return idCache[ objectId ]
end

function restore( spaceStore, objModel, revision )
	spaceStore:open()
	
	local rootId = spaceStore:getRootObject( revision )
	
	local obj = restoreObject( spaceStore, objModel, rootId, revision )
	
	local appliedUpdates = spaceStore:getUpdates( revision )
	local availableUpdates = objModel.updates
	
	local hasApplied = {}

	for script in appliedUpdates:gmatch( "[^;]+" ) do
		   hasApplied[script] = true
	end

	
	for _, script in ipairs( availableUpdates ) do
		if not hasApplied[script] then
		   print( script )
		end
	end
	
	spaceStore:close()
	
	coralObj = convertToCoral( obj )
	
	return coralObj
end

local coNew = co.new
local coType = co.Type

function convertToCoral( obj )
	if coralCache[obj._id] == nil then
		local root = coNew( obj._type )
		
		local currentService
		
		coralCache[ obj._id ] = root
		
		for k,v in pairs( obj ) do
			
			if k ~= "_type" and k ~= "_id" then
				currentService = root[k]
				if isFacet( k ) and currentService ~= nil then
					fillServiceValues( currentService, obj[k] )
				else
					if coralCache[ v._id ] == nil then
						convertToCoral( idCache[ v._provider ] )
						currentService = coralCache[ v._id ]
					end
					currentService = coralCache
				end
			end
		end
	end
	return coralCache[ obj._id ]
end

function isFacet( service )
	return true
end

function fillServiceValues( service, serviceValues )
	for sk, sv in pairs( serviceValues ) do
		if sk ~= "_type" and sk ~= "_id" and sk ~= "_provider" then
			if type( sv ) == 'table' and sv._type ~= nil then -- ref
				local objProvider = convertToCoral( idCache[sv._provider] )
				service[sk] = coralCache[sv._id]
			elseif type( sv ) == 'table' and #sv > 0 then -- refVec
				local serviceArray = {}
				for _, svInd in ipairs( sv ) do
					convertToCoral( idCache[svInd._provider] )
					serviceArray[ #serviceArray + 1 ] = coralCache[ svInd._id ]
				end
				service[sk] = serviceArray
			elseif type( sv ) == 'table' then --struct
				local struct = service[sk]
				for k, v in pairs( sv ) do
					struct[k] = v
				end
				service[sk] = struct
			else
				service[sk] = sv
			end
		end
	end
	
	coralCache[serviceValues._id] = service
end

local coRaise = co.raise
local function protectedRestoreSpace( space, spaceStore, objModel, revision )
	local ok, result = pcall( restore, spaceStore, objModel, revision )
	if not ok then
		coRaise( "ca.ModelException", result )
	end
	space:setRootObject( result )
end

return protectedRestoreSpace