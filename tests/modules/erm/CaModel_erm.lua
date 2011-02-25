-- Calcium Object Model description for module "erm"

Type "erm.Entity"
Type "erm.Model"
Type "erm.Relationship"

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
