-- Calcium Object Model description for module "serialization"

Type "serialization.SimpleEnum"

Type "serialization.BasicTypesStruct"
{
	byteValue = "int8",
	strValue = "string",
	intValue = "int32",
}

Type "serialization.NestedStruct"
{
	int16Value = "int16",
	structValue = "serialization.BasicTypesStruct",
}

Type "serialization.ArrayStruct"
{
	enums = "serialization.SimpleEnum[]",
	basicStructs = "serialization.BasicTypesStruct[]",
}

Type "serialization.NativeClassCoral"
{
	byteValue = "int8",
	intValue = "int32",
	doubleValue = "double",
}

Type "serialization.TwoLevelNestedStruct"
{
	boolean = "bool",
	nativeClass = "serialization.NativeClassCoral",
	nested = "serialization.NestedStruct",
}