-- this is used for testing duplicate definitions in CaModels
-- camodels/CaModel_extramodule.lua also defines the type 'erm.IEntity'

Type "camodels.SomeStruct"

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
