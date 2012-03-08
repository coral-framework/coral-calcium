local idCache = {}
local coralCache = {}

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
	if idCache[serviceId] == nil then
	
		local typeName = spaceStore:getObjectType( serviceId )
		
		namespaces[ extractNamespaceFullName( typeName ) ] = true
		
		local luaObjectTable = { _type = typeName, _id = serviceId }
		idCache[ serviceId ] = luaObjectTable
		
		local fieldNames = {}
		local values = {}
		fieldNames, values = spaceStore:getValues( serviceId, revision )
		
		for i, value in ipairs( values ) do
				local idServiceStr = value:sub( 2 )
			if  refKind( value ) == 1 then
				local chunk = load( "return " .. idServiceStr )
				local idServiceFieldValue = chunk()
				
				if idServiceFieldValue == nil then
					luaObjectTable[ fieldNames[i] ] = nil			
				else
					local objProvider = spaceStore:getServiceProvider( idServiceFieldValue )
					
					restoreObject( spaceStore, objModel, objProvider, revision )
					
					luaObjectTable[ fieldNames[i] ] = idCache[ idServiceFieldValue ]
				end
			elseif refKind( value ) == 2 then
				local idServiceListStr = value:sub( 2 )
				local chunk = load( "return " .. idServiceListStr )
				local idServiceList = chunk()
				
				local serviceList = {}
				
				for i, idService in ipairs( idServiceList ) do
					local objProvider = spaceStore:getServiceProvider( idService )
				
					restoreObject( spaceStore, objModel, objProvider, revision )
					
					serviceList[ #serviceList + 1 ] = idCache[ idService ]
				end
				
				luaObjectTable[ fieldNames[i] ] = serviceList
				
			else
				local chunk = load( "return " .. value )
				luaObjectTable[ fieldNames[i] ] = chunk()
			end
		end
		luaObjectTable._providerTable = idCache[ objectId ]
	end
	
	return idCache[ serviceId ]
end

function restoreObject( spaceStore, objModel, objectId, revision )
	if idCache[objectId] == nil then
		local typeName = spaceStore:getObjectType( objectId )
		
		local luaObjectTable = { _type = typeName, _id = objectId }
		idCache[ objectId ] = luaObjectTable
		
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
			applyUpdate( script, obj, spaceLoader )
			appliedUpdates = appliedUpdates .. script ..";"
		end
	end
	
	spaceStore:close()

	coralObj = convertToCoral( obj )
	
	space:setRootObject( coralObj )
	
	for k,v in pairs( coralCache ) do
		spaceLoader:insertObjectCache( v, k )
	end
	spaceLoader:setUpdateList( appliedUpdates )
end

function loadCaModels( objModel )
	for ns, _ in pairs( namespaces ) do
		objModel:loadDefinitionsFor( ns )
	end

end

function applyUpdate( updateScript, coralGraph, spaceLoader )
	local path = co.findScript( updateScript )
	
	local file, err = io.open( path, 'rb' )
	if file then
		local chunk, err = load( file:lines( 4096 ), path, 't' )
		
		if ( chunk ) then
			file:close()
			chunk()
			local ok, result = pcall( update, coralGraph, spaceLoader )
			
			if not ok then
				coRaise( "ca.IOException", result )
			end
			return result
		else
			coRaise( "ca.IOException", err )
		end
    end
	return nil
    
end

function convertToCoral( obj )
	if coralCache[obj._id] == nil then
		local root = coNew( obj._type )
		
		local currentService
		
		coralCache[ obj._id ] = root
		
		for k,v in pairs( obj ) do
			
			if k ~= "_type" and k ~= "_id" then
				currentService = root[k]
				if currentService ~= nil then
					fillServiceValues( currentService, obj[k] )
				else
					if coralCache[ v._id ] == nil then
						convertToCoral( v._providerTable )
						currentService = coralCache[ v._id ]
					end
					currentService = coralCache
				end
			end
		end
	end
	return coralCache[ obj._id ]
end

function fillServiceValues( service, serviceValues )
	coralCache[serviceValues._id] = service
	for sk, sv in pairs( serviceValues ) do
		if sk ~= "_type" and sk ~= "_id" and sk ~= "_providerTable" and sv ~= nil then
			if type( sv ) == 'table' and sv._type ~= nil then -- ref
				local objProvider = convertToCoral( sv._providerTable )
				service[sk] = coralCache[sv._id]
			elseif type( sv ) == 'table' and #sv > 0 then -- refVec
				local serviceArray = {}
				for _, svInd in ipairs( sv ) do
					convertToCoral( svInd._providerTable )
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
	
end


local function protectedRestoreSpace( space, spaceStore, objModel, revision, spaceLoader )
	idCache = {}
	coralCache = {}

	local ok, result = pcall( restore, space, spaceStore, objModel, revision, spaceLoader )
	if not ok then
		coRaise( "ca.IOException", result )
	end
end

return protectedRestoreSpace