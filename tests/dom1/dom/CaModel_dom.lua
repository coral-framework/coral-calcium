Type "dom.Company"
{
	company = "dom.ICompany",
}

Type "dom.Developer"
{
	developer = "dom.IDeveloper",
}

Type "dom.Manager"
{
	manager = "dom.IManager",
}

Type "dom.Project"
{
	project = "dom.IProject",
}

Type "dom.ICompany"
{
	projects = "dom.IProject[]",
}

Type "dom.IDeveloper"
{
	name = "string",
	salary = "double",
}

Type "dom.IManager"
{
	name = "string",
	salary = "double",
}

Type "dom.IProject"
{
	name = "string",
	isService = "bool",
	earnings = "double",
	manager = "dom.IManager",
	developers = "dom.IDeveloper[]",
}