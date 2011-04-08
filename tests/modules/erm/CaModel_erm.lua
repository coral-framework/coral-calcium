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
	relationship = "erm.IRelationship"
}

Type "erm.Multiplicity"
{
	min = "int32",
	max = "int32",
}

Type "erm.IEntity"
{
	name = "string",
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
