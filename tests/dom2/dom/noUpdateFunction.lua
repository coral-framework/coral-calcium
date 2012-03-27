		
function shouldBeUpdate( node )
	
	if node ~= nil then
		if node._type == "dom.Company" then
			node.projects = nil
			node.products = {}
			node.services = {}
		end
	end
	
end
