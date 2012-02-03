-- single-module model containing potential cycles

Type "camodels.SomeEnum"

Type "camodels.SomeInterface"
{
	str1 = "string",
	enum1 = "camodels.SomeEnum",
	struct1 = "camodels.SomeStruct",
	strArray = "string[]",
	autoRef = "camodels.SomeInterface",
}

Type "camodels.SomeStruct"
{
	int1 = "int32",
	str1 = "string",
	enum1 = "camodels.SomeEnum",
}

Type "erm.Model"

Update {"script"}
