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

Type "dom.Service"
{
	service = "dom.IService",
}

Type "dom.ICompany"
{
	products = "dom.IProduct[]",
	services = "dom.IService[]",
}

Type "dom.IEmployee"
{
	name = "string",
	salary = "int32",
	role = "string",
}

Type "dom.IProduct"
{
	name = "string",
	value = "double",
	leader = "dom.IEmployee",
	developers = "dom.IEmployee[]",
}

Type "dom.IService"
{
	name = "string",
	monthlyIncome = "double",
	mantainers = "dom.IEmployee[]",
}

Update "notFound"