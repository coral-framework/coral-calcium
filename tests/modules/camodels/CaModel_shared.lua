Type "camodels.SomeEnum"

Type "camodels.SomeComponent"
{
	someInterface = "camodels.SomeInterface"
}

Type "camodels.SomeInterface"
{
	str1 = "string",
	enum1 = "camodels.SomeEnum",
	struct1 = "camodels.SomeStruct",
	--strArray = "string[]",
	external = "erm.IModel"
}

Type "camodels.SomeStruct"
{
	int1 = "int32",
	str1 = "string",
	enum1 = "camodels.SomeEnum",
	--structWithinStruct = "erm.Multiplicity",
}