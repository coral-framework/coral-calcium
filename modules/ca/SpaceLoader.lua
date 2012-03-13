local idCache = {}
local conversionCache = {}

local namespaces = {}

local coNew = co.new
local coType = co.Type
local coRaise = co.raise

-- create private index
local index = {}

-- create metatable
local mt = {
	__index = function (t,k)
		local returnValue = t[index][k]
		return returnValue
	end,
	__newindex = function (t,k,v)
		if t[index][k] ~= nil and v == nil then
			t[index]["_id"] = nil
		elseif k == "_type" then
			t[index]["_id"] = nil
			-- re-type then is a new object to be add
		end
		t[index][k] = v   -- update original table
	end
}

function track (t)
  local proxy = {}
  proxy[index] = t
  setmetatable(proxy, mt)
  return proxy
end


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
		luaObjectTable = track( luaObjectTable )
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
		luaObjectTable = track( luaObjectTable )
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
			applyUpdate( script, track( obj ), spaceLoader )
			appliedUpdates = appliedUpdates .. script ..";"
		end
	end
	
	spaceStore:close()

	coralObj = convertToCoral( obj, objModel )
	
	space:setRootObject( coralObj )
	space:notifyChanges()
	
	for k, v in pairs( conversionCache ) do
		if k._id == nil then
			spaceLoader:insertNewObject( v )
		else
			spaceLoader:insertObjectCache( v, k._id )
		end
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

function convertToCoral( obj, objModel )
	if conversionCache[obj] == nil then
		local root = coNew( obj._type )
	
		local currentService
		
		conversionCache[ obj ] = root
		local ports = objModel:getPorts( root.component )
		local portName
		for i, port in ipairs( ports ) do
			portName = port.name
			currentService = root[portName]
			currentServiceValues = obj[portName]
			if port.isFacet then
				fillServiceValues( currentService, currentServiceValues, objModel )
			else
				if conversionCache[ currentServiceValues ] == nil then
					local providerObj = convertToCoral( currentServiceValues._providerTable, objModel )
					currentService = conversionCache[ currentServiceValues ]
					root[port] = currentService
				end
			end
		end
	end
	return conversionCache[ obj ]
end

function fillServiceValues( service, serviceValues, objModel )
	conversionCache[serviceValues] = service
	
	local fields = objModel:getFields( service.interface )
	
	local fieldKind
	local fieldValue
	local fieldName
	
	for i, field in ipairs( fields ) do
		fieldKind = field.type.kind
		fieldName = field.name
		fieldValue = serviceValues[field.name]
		
		if fieldValue ~= nil then
			if fieldKind == 'TK_INTERFACE' then
				local objProvider = convertToCoral( fieldValue._providerTable, objModel )
				service[fieldName] = conversionCache[fieldValue]
				
			elseif fieldKind == 'TK_ARRAY' then
				local elementType = field.type.elementType
				if elementType.kind == 'TK_INTERFACE' then
					local serviceArray = {}
					for _, svInd in ipairs( fieldValue ) do
						convertToCoral( svInd._providerTable, objModel )
						serviceArray[ #serviceArray + 1 ] = conversionCache[ svInd ]
					end
					service[fieldName] = serviceArray
				else
					service[fieldName] = fieldValue
				end
			elseif fieldKind == 'TK_STRUCT' then
				local struct = service[fieldName]
				for k, v in pairs( fieldValue ) do
					struct[k] = v
				end
				service[fieldName] = struct
			else
				service[fieldName] = fieldValue
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