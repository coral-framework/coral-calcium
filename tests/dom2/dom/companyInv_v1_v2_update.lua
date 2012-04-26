
function companyUpdate( node )
	local company = node.company
	
	local projects = company.projects
	local employeeList = {}
	
	for _, project in ipairs( projects ) do
		local projectUpdated = project._providerTable
		update( projectUpdated )
		for _, dev in ipairs( project.developers ) do
			dev.working = {}
			update( dev._providerTable )
			dev.working[ #dev.working + 1 ] = getCorrectService( projectUpdated )
			employeeList[ #employeeList + 1 ] = dev
		end
		if( getCorrectService( projectUpdated )._type == "dom.IProduct" ) then
			local managerEmployee = project.manager._providerTable
			convertToEmployee( managerEmployee )
			managerEmployee.employee.leading = getCorrectService( projectUpdated )
			employeeList[ #employeeList + 1 ] = managerEmployee.employee
			-- service ignore the manager, removing object
		end
		
	end
	
	local ceo = {
					_type = "dom.Employee",
					employee = 
					{
						_type = "dom.IEmployee",
						name = "James CEO Boss",
						salary = 100000.0,
						role = "CEO",
						working = {},
					}
				}
				
	ceo.employee._providerTable = ceo
	
	node.ceo = ceo.employee
	company.employees = employeeList
	company.projects = nil
end

function getCorrectService( project )

	if project.service == nil then
		return project.product
	else
		return project.service
	end

end

function projectUpdate( node )
	local devs = {}
	
	local project = node.project
	if ( project.isService ) then

		node._type = "dom.Service"
		
		node.service = {
						_type = "dom.IService", 
						name = project.name,
						monthlyIncome = project.earnings / 1.90, --value conversion, now in us dollars.
						_providerTable = node,
					}
		
		node.project = nil
	else
		node._type = "dom.Product"
		
		node.product = {
						_type = "dom.IProduct",
						name = project.name,
						value = project.earnings, 
						_providerTable = node,
					}
		leaderObj = project.manager._providerTable
		node.product.leader = leaderObj.employee
		node.project = nil

	end
	
end

function convertToEmployee( node )
	if node._type == "dom.Manager" then
		node.employee = node.manager
		node.employee.role = "Manager"
		node.manager = nil
	else
		node.employee = node.developer
		node.employee.role = "Developer"
		node.developer = nil
	end
	node.employee._type = "dom.IEmployee"
	node._type = "dom.Employee"
		
end

local updateFunctions
updateFunctions = { ["dom.Company"] = companyUpdate, ["dom.Manager"] = convertToEmployee, ["dom.Developer"] = convertToEmployee, ["dom.Project"] = projectUpdate,}
		
function update( node )
	
	if node ~= nil then
		local updateFunc = updateFunctions[ node._type ]
		
		updateFunc( node )
	end
	
end
