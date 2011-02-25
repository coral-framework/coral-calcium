-- single-module model containing potential cycles

Type "camodels.SomeEnum"

Type "camodels.SomeInterface"
{
	str1 = "string",
	enum1 = "camodels.SomeEnum",
	struct1 = "camodels.SomeStruct",
	strArray = "string[]",
}

Type "camodels.SomeStruct"
{
	int1 = "int32",
	str1 = "string",
	enum1 = "camodels.SomeEnum",
	interface1 = "camodels.SomeInterface",
}
