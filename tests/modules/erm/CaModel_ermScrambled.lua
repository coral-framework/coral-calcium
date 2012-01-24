-- Calcium Object Model description for module "erm"

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
	entityB = "erm.IEntity",
	entityA = "erm.IEntity",
	relationship = "erm.IRelationship",
}

Type "erm.Multiplicity"
{
	max = "int32",
	min = "int32",
}

Type "erm.IEntity"
{
	parent = "erm.IEntity",
	name = "string",
}

Type "erm.IModel"
{
	relationships = "erm.IRelationship[]",
	entities = "erm.IEntity[]",
}

Type "erm.IRelationship"
{
	entityA = "erm.IEntity",
	multiplicityA = "erm.Multiplicity",
	entityB = "erm.IEntity",
	multiplicityB = "erm.Multiplicity",
	relation = "string",
}
