
function companyUpdate( node, spaceLoader )
	local companyFacet = node.company
	
	local projects = companyFacet.projects
	
	node.company.projects = nil
	
	local serviceList = {}
	local productList = {}
	
	for _, project in ipairs( projects ) do
		update( project._providerTable, spaceLoader )
		local projectUpdated = project._providerTable
		if( project.isService ) then
			serviceList[ #serviceList + 1 ] = projectUpdated.service
		else
			productList[ #productList + 1 ] = projectUpdated.product
		end
	end
	
	node.company.services = serviceList
	node.company.products = productList
end

function projectUpdate( node, spaceLoader )
	local devs = {}
	
	local project = node.project
	
	for _, dev in ipairs( project.developers ) do
		devProvider = dev._providerTable
		update( devProvider, spaceLoader )
		devs[ #devs + 1 ] = devProvider.employee
	end
		
	if ( project.isService ) then

		node._type = "dom.Service"
		
		devProvider = project.manager._providerTable
		update( devProvider, spaceLoader )
		devs[ #devs + 1 ] = devProvider.employee
		
		node.service = {
						_type = "dom.IService", 
						name = project.name,
						monthlyIncome = project.earnings,
						mantainers = devs,
						_providerTable = node,
						_id = node.project._id,
					}
		
		node.project = nil
	else
		node._type = "dom.Product"
		
		node.product = {
						_type = "dom.IProduct",
						name = project.name,
						value = project.earnings,
						developers = devs,
						_providerTable = node,
						_id = node.project._id,
					}
		leaderObj = project.manager._providerTable
		update( leaderObj, spaceLoader )
		node.product.leader = leaderObj.employee
		node.project = nil

	end
	
end

function convertToEmployee( node, spaceLoader )

	if( node._type == "dom.Manager" ) then
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

function update( node, spaceLoader )
	if( node ~= nil ) then
		local updateFunctions = {} 
		
		updateFunctions[ "dom.Company" ] = companyUpdate
		updateFunctions[ "dom.Manager" ] = convertToEmployee
		updateFunctions[ "dom.Developer" ] = convertToEmployee
		updateFunctions[ "dom.Project" ] = projectUpdate
		
		local updateFunc = updateFunctions[ node._type ]
		
		updateFunc( node, spaceLoader )
		
	end
	
end
