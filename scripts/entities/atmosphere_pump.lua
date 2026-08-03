local maxOutput = 100 -- Per gas

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "computer")
classData:Set("spawnable_collider", Vec3(0.75))

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(Vec3(0.75)),
		objectLayer = Constants.ObjectLayerDynamic
	}

	self:AddComponent("rigidbody3d", physSettings)

	self:AddComponent("distribution", {
		inputs = { DistributionType.Electrical },
		outputs = { DistributionType.Gas }
	})
	
	if CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText("Pump")

		local model = AssetLibrary.GetModel("computer")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end

	if SERVER then
		self:AddComponent("atmosphere_monitor")
	end
end)

if SERVER then
	local allGases = {GasType.Oxygen, GasType.CarbonDioxyde, GasType.Nitrogen}
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")

		local output = GasQuantity()

		local monitor = self:GetComponent("atmosphere_monitor")
		if not monitor.Atmosphere then
			distribution:UpdateProductionValue(0, output)
			return
		end

		local gases = {}
		for _, gasType in ipairs(allGases) do
			gases[gasType] = -math.min(monitor.Atmosphere:GetGasAmount(gasType), maxOutput)
		end

		if not monitor.Atmosphere:Exchange(gases) then
			distribution:UpdateProductionValue(0, output)
			return
		end

		for gasType, quantity in pairs(gases) do
			output:Increment(gasType, Distribution.ToTickUnit(quantity))
		end
		distribution:UpdateProductionValue(0, output)
	end)
end

EntityRegistry.RegisterClass("atmosphere_pump", classData)
