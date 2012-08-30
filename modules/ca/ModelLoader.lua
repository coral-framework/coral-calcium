-------------------------------------------------------------------------------
-- Lua module for loading object model description files (CaModel_*.lua)
-------------------------------------------------------------------------------

local function loadFileIn( filePath, env )
	local file, err = io.open( filePath, 'rb' )
	if file then
		local chunk, err = load( file:lines( 4096 ), filePath, 't', env )
		file:close()
		return chunk, err
    end
    return file, err
end

-------------------------------------------------------------------------------
-- CaModel DSL Environment

local coType = co.Type

local caModelEnv = {}
local caModelEnvMT = { __index = caModelEnv }
local currentEnv = nil
local updateList = {}

local typeMT = { __call = function( typeDecl, entries )
	typeDecl.entries = entries
end }

function caModelEnv.Type( name )
	local ok, type = pcall( coType, name )
	if not ok then
		error( "could not load type '" .. tostring( name ) .. "'", 0 )
	end
	local typeDecl = setmetatable( { type = type, name = name }, typeMT )
	currentEnv[#currentEnv + 1] = typeDecl
	return typeDecl
end

function caModelEnv.Update( updateScript )
	if type( updateScript ) ~= 'string' then
		error( "illegal script name '" .. tostring( updateScript ) .. "'", 0 )
	end

	updateList[#updateList + 1] = updateScript
end

-------------------------------------------------------------------------------
-- This function traverses an array of typeDecls, performs some basic checks
-- and adds the types to the objModel.

local isRecordType = {
	TK_STRUCT = true,
	TK_INTERFACE = true,
	TK_NATIVECLASS = true,
}

local function processTypes( types, objModel )
	for i, typeDecl in ipairs( types ) do
		local type = typeDecl.type
		local kind = type.kind
		local entries = typeDecl.entries or {}
		if kind == 'TK_ARRAY' then
			error( "array type '" .. typeDecl.name .. "' cannot be defined in the object model; define '"
						.. type.elementType.fullName .. "' instead", 0 )
		elseif kind == 'TK_ENUM' then
			if entries and next( entries ) then
				error( "enum '" .. typeDecl.name .. "' should not have a field list", 0 )
			end
			objModel:addEnum( type )
		elseif kind == 'TK_EXCEPTION' then
			error( "exception type '" .. typeDecl.name .. "' cannot be defined in the object model", 0 )
		elseif kind == 'TK_COMPONENT' then
			-- resolve the list of ports
			local ports = {}
			for portName, expectedTypeName in pairs( entries ) do
				local port = type:getMember( portName )
				if not port then
					error( "no such port '" .. portName .. "' in component '" .. typeDecl.name .. "'", 0 )
				end
				local portType = port.type
				if portType.fullName ~= expectedTypeName then
					error( "port '" .. portName .. "' in component '" .. typeDecl.name ..
						"' was expected to be of type '" .. expectedTypeName ..
						"', but it is really of type '" .. portType.fullName .. "'", 0 )
				end
				ports[#ports + 1] = port
			end
			objModel:addComponent( type, ports )
		elseif isRecordType[kind] then
			-- resolve the list of fields
			local fields = {}
			for fieldName, expectedTypeName in pairs( entries ) do
				local field = type:getMember( fieldName )
				if not field then
					error( "no such field '" .. fieldName .. "' in type '" .. typeDecl.name .. "'", 0 )
				end
				local fieldType = field.type
				if fieldType.fullName ~= expectedTypeName then
					error( "field '" .. fieldName .. "' in type '" .. typeDecl.name ..
						"' was expected to be of type '" .. expectedTypeName ..
						"', but it is really of type '" .. fieldType.fullName .. "'", 0 )
				end
				-- fields read-only are now allowed, you may want listen changes on them.
				-- it will throw IllegalArgumentException when you try to save/restore such field
				-- if field.isReadOnly then
					-- error( "illegal read-only field '" .. fieldName .. "' defined for type '" .. typeDecl.name .. "'", 0 )
				-- end
				fields[#fields + 1] = field
			end
			objModel:addRecordType( type, fields )
		else
			error( "cannot define primitive type '" .. typeDecl.name .. "'", 0 )
		end
	end
end

-------------------------------------------------------------------------------
-- This function loads a CaModel file in the context of an objModel.

local function loadModelFile( objModel, filePath )
	currentEnv = setmetatable( {}, caModelEnvMT )
	updateList = {}

	local chunk, err = loadFileIn( filePath, currentEnv )
	if not chunk then
		error( err )
	end
	
	chunk()
	
	for _, script in ipairs( updateList ) do
		objModel:addUpdate( script )
	end

	processTypes( currentEnv, objModel )
end

local coRaise = co.raise
local function protectedLoadModelFile( objModel, filePath )
	local ok, err = pcall( loadModelFile, objModel, filePath )
	currentEnv = nil
	if not ok then
		coRaise( "ca.ModelException", err )
	end
end

return protectedLoadModelFile
