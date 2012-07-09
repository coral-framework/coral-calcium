-- Calcium Object Model description for module "erm"

Type "erm.Entity"
{
	entity = "erm.IEntity"
}

Type "erm.Model"
{
	model = "erm.IModel"
}

Type "erm.IEntity"
{
	name = "string",
	parent = "erm.IEntity",
}

Type "erm.IModel"
{
	entities = "erm.IEntity[]",
}
