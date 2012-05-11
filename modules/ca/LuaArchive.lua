--[[----------------------------------------------------------------------------

	ca.LuaArchive component

--]]----------------------------------------------------------------------------

local io = io
local type = type
local pairs = pairs
local ipairs = ipairs
local load = load
local rawget = rawget
local tostring = tostring

local coNew = co.new
local coRaise = co.raise
local coTypeOf = co.typeOf
local coType = co.Type

--------------------------------------------------------------------------------
-- Calcium Model Wrappers
--------------------------------------------------------------------------------

local ModelWrapper = {}  -- the ModelWrapper class
local wrappedModels = {} -- database of ModelWrappers, indexed by model name

function ModelWrapper:__index( typeName )
	if type( typeName ) ~= 'string' then
		typeName = coTypeOf( typeName )
	end
	local v = rawget( self, typeName )
	if not v then
		local t = coType[typeName]
		if t.kind == 'TK_COMPONENT' then
			v = self.__service:getPorts( t )
		else
			v = self.__service:getFields( t )
		end
		self[typeName] = v
	end
	return v
end

function ModelWrapper:wrap( model )
	local name = model.name
	assert( #name > 0 )
	local self = wrappedModels[name]
	if not self then
		self = setmetatable( { __service = model }, ModelWrapper )
		wrappedModels[name] = self
	end
	return self
end

--------------------------------------------------------------------------------
-- Save functions
--------------------------------------------------------------------------------

local saveRef, saveValue, saveArray, saveField

saveRef = function( writer, model, index, service )
	if not service then writer( "nil" ) return end
	local provider = service.provider
	local id = index[provider]
	if not id then
		id = #index + 1
		index[id] = provider
		index[provider] = id
	end
	writer( "Ref{ id = ", id, ", facet = '", service.facet.name, "' }" )
end

saveValue = function( writer, model, index, value, valueType, indentLevel, indent )
	local tp = type( value )
	local valueTypeName = valueType.fullName
	if tp == 'userdata' then
		writer( 'Value "', valueTypeName,'" {\n' )
		for i, field in ipairs( model[valueTypeName] ) do
			saveField( writer, model, index, value, field, indentLevel )
		end
		writer( indent, "}" )
	elseif tp == 'string' then
		writer( ( "%q" ):format( value ) )
	else
		writer( tostring( value ) )
	end
end

saveArray = function( writer, model, index, array, elementType, indentLevel, indent )
	writer( 'ArrayOf "', elementType.fullName, '" {\n' )
	local saveElement = saveValue
	if elementType.kind == 'TK_INTERFACE' then
		saveElement = saveRef
	end
	for i, element in ipairs( array ) do
		writer( indent, "\t" )
		saveElement( writer, model, index, element, elementType, indentLevel, indent )
		writer( ",\n" )
	end
	writer( indent, "}" )
end

saveField = function( writer, model, index, composite, field, indentLevel )
	indentLevel = ( indentLevel or 1 ) + 1
	local indent = ( '\t' ):rep( indentLevel )

	local name = field.name
	local type = field.type
	local kind = type.kind
	local value = composite[name]
	writer( indent, name, " = " )

	if kind == 'TK_INTERFACE' then
		saveRef( writer, model, index, value )
	elseif kind == 'TK_ARRAY' then
		saveArray( writer, model, index, value, type.elementType, indentLevel, indent )
	else
		saveValue( writer, model, index, value, type, indentLevel, indent )
	end

	writer( ",\n" )
end

local function saveService( writer, model, index, object, port )
	local service = object[port.name]
	local typeName = port.type.fullName
	writer( 'Service "', typeName, '" {\n' )
	for i, field in ipairs( model[typeName] ) do
		saveField( writer, model, index, service, field )
	end
	writer( "\t}" )
end

local function saveObject( writer, model, index, object )
	local typeName = coTypeOf( object )
	local currentId = index[object]

	writer( '\nobjects[', currentId, '] = Object "', typeName, '" {\n' )

	for i, port in ipairs( model[typeName] ) do
		writer( "\t", port.name, " = " )
		if port.isFacet then
			saveService( writer, model, index, object, port )
		else
			saveRef( writer, model, index, object[port.name] )
		end
		writer( ",\n" )
	end

	writer( "}\n" )

	-- save the next object, if there is one
	object = index[currentId + 1]
	if object then return saveObject( writer, model, index, object ) end
end

--------------------------------------------------------------------------------
-- Restore functions
--------------------------------------------------------------------------------

local archiveEnv = {}
local archiveEnvMT = { __index = archiveEnv }

local recordMT = { __call = function( t, data )
	t.data = data
	return t
end }

local function defineRecord( kind )
	return function( typeName )
		return setmetatable( { kind = kind, typeName = typeName }, recordMT )
	end
end

archiveEnv.Object = defineRecord 'object'
archiveEnv.Service = defineRecord 'service'
archiveEnv.ArrayOf = defineRecord 'array'
archiveEnv.Value = defineRecord 'value'
archiveEnv.Ref = function( t ) return t end

local function resolveRef( objects, ref )
	if ref ~= nil then
		return objects[ref.id].object[ref.facet]
	end
end

local function resolveValue( objects, value )
	-- value needs processing?
	if type( value ) == 'table' then
		local kind = value.kind
		if kind == 'array' then
			value = value.data
			-- array needs processing?
			if type( value[1] ) == 'table' then
				local array = {}
				for i, v in ipairs( value ) do
					array[i] = resolveValue( objects, v )
				end
				value = array
			end
		elseif kind == 'value' then
			local complexValue = coNew( value.typeName )
			for fieldName, v in pairs( value.data ) do
				complexValue[fieldName] = resolveValue( objects, v )
			end
			value = complexValue
		else
			value = resolveRef( objects, value )
		end
	end
	return value
end

local function restoreObjects( objects )
	-- instantiate all objects
	for i, objRec in ipairs( objects ) do
		objRec.object = coNew( objRec.typeName )
	end

	-- restore all values and references
	for i, objRec in ipairs( objects ) do
		local object = objRec.object
		for portName, portRec in pairs( objRec.data ) do
			if portRec.kind == 'service' then
				local service = object[portName]
				for fieldName, valueRec in pairs( portRec.data ) do
					service[fieldName] = resolveValue( objects, valueRec )
				end
			else
				object[portName] = resolveRef( objects, portRec )
			end
		end
	end

	return objects[1].object
end

--------------------------------------------------------------------------------
-- ca.LuaArchive
--------------------------------------------------------------------------------

local LuaArchive = co.Component "ca.LuaArchive"

function LuaArchive:checkConfig()
	if not self.model then
		coRaise( "ca.ModelException", "the ca.LuaArchive requires a model for this operation" )
	end
	if not self.filename then
		coRaise( "ca.IOException", "the ca.LuaArchive requires a file name for this operation" )
	end
end

function LuaArchive:save( rootObject )
	self:checkConfig()

	assert( self.model[rootObject] )

	local f, errormsg = io.open( self.filename, "w" )
	if not f then coRaise( "ca.IOException", "error saving the file: " .. errormsg ) end
	local write = f.write
	local writer = function(...) write( f, ... ) end

	writer( [[
--------------------------------------------------------------------------------
-- Lua Archive File
-- Saved on ]], os.date "%a %b %d %H:%M:%S %Y", [[.
--------------------------------------------------------------------------------

-- Metadata
format_version = 1

-- Archived Objects
]] )

	local index = {}
	index[1] = rootObject
	index[rootObject] = 1

	saveObject( writer, self.model, index, rootObject )

	f:close()
end

function LuaArchive:restore()
	self:checkConfig()

	local file, err = io.open( self.filename, 'rb' )
	if not file then
		coRaise( "ca.IOException", err )
	end

	local env = setmetatable( { objects = {} }, archiveEnvMT )

	local chunk, err = load( file:lines( 4096 ), filePath, 't', env )
	file:close()

	if not chunk then
		coRaise( "ca.FormatException", err )
	end

	chunk()

	return restoreObjects( env.objects )
end

function LuaArchive:getName( name )
	return self.filename or ""
end

function LuaArchive:setName( name )
	self.filename = name
end

function LuaArchive:getModelService( model )
	return self.model
end

function LuaArchive:setModelService( model )
	self.model = ModelWrapper:wrap( model )
end

return LuaArchive
