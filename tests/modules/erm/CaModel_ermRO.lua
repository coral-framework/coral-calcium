-- Just like "erm" but with added read-only fields
-- This means this model is only for change tracking, not for persistence

Type "erm.Entity"
{
	entity = "erm.IEntity"
}

Type "erm.Model"
{
	model = "erm.IModel"
}

Type "erm.Relationship"
{
	relationship = "erm.IRelationship",
	entityA = "erm.IEntity",
	entityB = "erm.IEntity",
}

Type "erm.Multiplicity"
{
	min = "int32",
	max = "int32",
}

Type "erm.IEntity"
{
	name = "string",
	parent = "erm.IEntity",
	adjacentEntityNames = "string[]",
}

Type "erm.IModel"
{
	entities = "erm.IEntity[]",
	relationships = "erm.IRelationship[]",
}

Type "erm.IRelationship"
{
	entityA = "erm.IEntity",
	entityB = "erm.IEntity",
	multiplicityA = "erm.Multiplicity",
	multiplicityB = "erm.Multiplicity",
	relation = "string",
}
