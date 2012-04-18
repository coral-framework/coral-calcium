Type "dom.Company"
{
	company = "dom.ICompany",
}

Type "dom.Employee"
{
	employee = "dom.IEmployee",
}

Type "dom.Product"
{
	product = "dom.IProduct",
}

Type "dom.IProject"
{
	name = "string",
}

Type "dom.Service"
{
	service = "dom.IService",
}

Type "dom.ICompany"
{
	employees = "dom.IEmployee[]",
}

Type "dom.IEmployee"
{
	name = "string",
	salary = "int32",
	role = "string",
	leading = "dom.IProject",
	working = "dom.IProject[]",
}

Type "dom.IProduct"
{
	name = "string",
	value = "double",
}

Type "dom.IService"
{
	name = "string",
	monthlyIncome = "double",
}

Update "dom.companyInv_v1_v2_update"