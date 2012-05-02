
function companyUpdate( node )
	local company = node.company
	
	local projects = company.projects
	local employeeList = {}
	
	for _, project in ipairs( projects ) do
		local projectUpdated = project._provider
		update( projectUpdated )
		for _, dev in ipairs( project.developers ) do
			dev.working = {}
			update( dev._provider )
			dev.working[ #dev.working + 1 ] = getCorrectService( projectUpdated )
			employeeList[ #employeeList + 1 ] = dev
		end
		
		if( getCorrectService( projectUpdated )._type == "dom.IProduct" ) then
			local managerEmployee = project.manager._provider
			convertToEmployee( managerEmployee )
			managerEmployee.employee.leading = getCorrectService( projectUpdated )
			employeeList[ #employeeList + 1 ] = managerEmployee.employee
			-- service ignore the manager, removing object
		end
		
	end

	local ceo = newInstance( "dom.Employee" )
	
	ceo.employee.name = "James CEO Boss"
	ceo.employee.salary = 100000.0
	ceo.employee.role = "CEO"
		
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
		
		node.service = Facet "dom.IService" {
									name = project.name,
									monthlyIncome = project.earnings / 1.90, --value conversion, now in us dollars.
								}
		
		node.project = nil
	else
		node._type = "dom.Product"
		
		node.product = Facet "dom.IProduct" {
							name = project.name,
							value = project.earnings, 
						}
						
		leaderObj = project.manager._provider
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
