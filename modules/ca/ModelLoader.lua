-------------------------------------------------------------------------------
-- Lua module for loading object model description files (CaModel_*.lua)
-------------------------------------------------------------------------------

-- Loads a chunk from a file and sets its _ENV
local function loadFileIn( filePath, env )
	local file, err = io.open( filePath, 'rb' )
	if file then
		local chunk, err = loadin( env, file:lines( 4096 ), filePath )
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

local typeMT = { __call = function( typeDecl, fields )
	typeDecl.fields = fields
end }

function caModelEnv.Type( name )
	local ok, type = pcall( coType, name )
	if not ok then
		error( "type '" .. tostring( name ) .. "' does not exist", 0 )
	end
	local typeDecl = setmetatable( { type = type, name = name }, typeMT )
	currentEnv[#currentEnv + 1] = typeDecl
	return typeDecl
end

-------------------------------------------------------------------------------
-- This function traverses an array of typeDecls, performs some basic checks
-- and adds the types to the objModel.

local isAttributeContainer = {
	TK_STRUCT = true,
	TK_INTERFACE = true,
	TK_NATIVECLASS = true,
}

local function processTypes( types, objModel )
	for i, typeDecl in ipairs( types ) do
		local type = typeDecl.type
		local kind = type.kind
		local fields = typeDecl.fields or {}
		if kind == 'TK_ARRAY' then
			error( "array type '" .. typeDecl.name .. "' cannot be defined in the object model; define '"
						.. type.elementType.fullName .. "' instead", 0 )
		elseif kind == 'TK_ENUM' then
			if fields and next( fields ) then
				error( "enum type '" .. typeDecl.name .. "' should not have a field list", 0 )
			end
			objModel:addEnum( type )
		elseif kind == 'TK_EXCEPTION' then
			error( "exception type '" .. typeDecl.name .. "' cannot be defined in the object model", 0 )
		elseif kind == 'TK_COMPONENT' then
			-- resolve the list of receptacles (InterfaceInfos)
			local receptacles = {}
			for itfName, expectedTypeName in pairs( fields ) do
				local itfInfo = type:getMember( itfName )
				if not itfInfo then
					error( "no such interface '" .. itfName .. "' in component '" .. typeDecl.name .. "'", 0 )
				elseif itfInfo.isFacet then
					error( "interface '" .. itfName .. "' of component '" .. typeDecl.name .. "' is not a receptacle", 0 )
				end
				local itfType = itfInfo.type
				if itfType.fullName ~= expectedTypeName then
					error( "interface '" .. itfName .. "' of component '" .. typeDecl.name ..
						"' was expected to be of type '" .. expectedTypeName ..
						"', but it is really of type '" .. itfType.fullName .. "'", 0 )
				end
				receptacles[#receptacles + 1] = itfInfo
			end
			objModel:addComponent( type, receptacles )
		elseif isAttributeContainer[kind] then
			-- resolve the list of attributes (AttributeInfos)
			local attributes = {}
			for attribName, expectedTypeName in pairs( fields ) do
				local attribInfo = type:getMember( attribName )
				if not attribInfo then
					error( "no such attribute '" .. attribName .. "' in type '" .. typeDecl.name .. "'", 0 )
				end
				local attribType = attribInfo.type
				if attribType.fullName ~= expectedTypeName then
					error( "attribute '" .. attribName .. "' of type '" .. typeDecl.name ..
						"' was expected to be of type '" .. expectedTypeName ..
						"', but it is really of type '" .. attribType.fullName .. "'", 0 )
				end
				if attribInfo.isReadOnly then
					error( "attribute '" .. attribInfo .. "' of type '" .. typeDecl.name .. "' is read-only", 0 )
				end
				attributes[#attributes + 1] = attribInfo
			end
			objModel:addAttributeContainer( type, attributes )
		else
			error( "primitive type '" .. typeDecl.name .. "' cannot be defined in the object model.", 0 )
		end
	end
end

-------------------------------------------------------------------------------
-- This function loads a CaModel file in the context of an objModel.

local function loadModelFile( objModel, filePath )
	currentEnv = setmetatable( {}, caModelEnvMT )

	local chunk, err = loadFileIn( filePath, currentEnv )
	if not chunk then
		error( err )
	end

	chunk()

	local types = currentEnv
	currentEnv = nil

	processTypes( types, objModel )
end

local coRaise = co.raise
local function protectedLoadModelFile( objModel, filePath )
	local ok, err = pcall( loadModelFile, objModel, filePath )
	if not ok then
		coRaise( "ca.ObjectModelException", err )
	end
end

return protectedLoadModelFile
