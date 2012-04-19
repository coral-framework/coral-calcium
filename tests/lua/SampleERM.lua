local M = {}

local model, universe, space

local entityA, entityB, entityC
local relAB, relBC, relCA
local erm

function M:setup()
	self.model = co.new( "ca.Model" ).model
	self.model.name = "erm"

	local object = co.new "ca.Universe"
	object.model = self.model
	self.universe = object.universe

	object = co.new "ca.Space"
	object.universe = self.universe
	self.space = object.space
end

function M:teardown()
	self.model, self.universe, self.space = nil, nil, nil
	self.entityA, self.entityB, self.entityC = nil, nil, nil
	self.relAB, self.relBC, self.relCA = nil, nil, nil
	self.erm = nil
end

function M:createERM()
	self.entityA = co.new( "erm.Entity" ).entity
	self.entityA.name = "Entity A"

	self.entityB = co.new( "erm.Entity" ).entity
	self.entityB.name = "Entity B"

	self.relAB = co.new( "erm.Relationship" ).relationship
	self.relAB.relation = "relation A-B"
	self.relAB.entityA = self.entityA
	self.relAB.entityB = self.entityB

	self.erm = co.new( "erm.Model" ).model
	self.erm:addEntity( self.entityA )
	self.erm:addEntity( self.entityB )
	self.erm:addRelationship( self.relAB )

	-- entityC and its rels are created but not added to the graph yet
	self.entityC = co.new( "erm.Entity" ).entity
	self.entityC.name = "Entity C"

	self.relBC = co.new( "erm.Relationship" ).relationship
	self.relBC.relation = "relation B-C"

	self.relCA = co.new( "erm.Relationship" ).relationship
	self.relCA.relation = "relation C-A"

	return self.erm
end

function M:extendERM()
	self.relBC.entityA = self.entityB
	self.relBC.entityB = self.entityC
	self.relCA.entityA = self.entityC
	self.relCA.entityB = self.entityA
	self.erm:addEntity( self.entityC )
	self.erm:addRelationship( self.relBC )
	self.erm:addRelationship( self.relCA )
end

function M:spaceWithSimpleERM()
	if not self.erm then self:createERM() end
	self.space:initialize( self.erm.provider )
	self.space:notifyChanges()
end

function M:spaceWithExtendedERM()
	if not self.erm then self:createERM() end
	self:extendERM()
	self.space:initialize( self.erm.provider )
	self.space:notifyChanges()
end

return M
