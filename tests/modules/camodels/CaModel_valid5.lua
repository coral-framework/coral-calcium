-- single-module model containing potential cycles

Update "script1.lua"

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

Update "script2.lua"

Type "erm.Model"

Update "script3.lua"
Update "script4.lua"
