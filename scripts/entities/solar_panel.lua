local modelSize = Vec3(2.0, 0.1, 2.0)

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "solar_panel")
classData:Set("spawnable_collider", modelSize)

classData:AddProperty("power_factor", { type = "float", default = 0.0, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(modelSize),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	local distribution = self:AddComponent("distribution", {
		inputs = {}, 
		outputs = { DistributionType.Electrical }
	})

	if SERVER then
		distribution:UpdateProductionValue(0, ElectricalQuantity(0))
	elseif CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText(string.format("Solar panel (%d %%)", math.floor(self:GetProperty("power_factor") * 100 + 0.5)))

		local model = AssetLibrary.GetModel("solar_panel")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	local sunDir = Vec3(0.8528685, 0.49999997, 0.15038377)

	classData:On("distribution", function (self)
		local environment = self:GetEnvironment()

		local correctedSunDir = sunDir
		if environment:GetType() == EnvironmentType.Ship then
			local exteriorEntity = environment:GetExteriorShipEntity()
			local exteriorNode = exteriorEntity:GetComponent("node")
			correctedSunDir = exteriorNode:GetRotation():GetConjugate() * correctedSunDir
		end

		local node = self:GetComponent("node")
		local powerFactor = math.max(node:GetUp():DotProduct(correctedSunDir), 0)

		local distribution = self:GetComponent("distribution")
		distribution:UpdateProductionValue(0, ElectricalQuantity(Distribution.ToTickUnit(math.floor(powerFactor * 100))))

		self:UpdateProperty("power_factor", powerFactor)
	end)
else
	classData:OnPropertyUpdate("power_factor", function (self, factor)
		self:SetInteractibleText(string.format("Solar panel (%d %%)", math.floor(factor * 100 + 0.5)))
	end)
end

EntityRegistry.RegisterClass("solar_panel", classData)
