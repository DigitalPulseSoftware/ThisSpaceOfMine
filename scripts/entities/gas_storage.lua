local maxInput = Distribution.ToTickUnit(200)
local maxOutput = Distribution.ToTickUnit(200)

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "battery")
classData:Set("spawnable_collider", Vec3(0.5, 1.1, 0.5))

classData:AddProperty("capacity", { type = "integer", default = Distribution.ToStorageUnit(10000), isNetworked = true })

-- TODO: Replace by RPC
classData:AddProperty("storage_o2", { type = "integer", default = 0, isNetworked = true })
classData:AddProperty("storage_co2", { type = "integer", default = 0, isNetworked = true })
classData:AddProperty("storage_n2", { type = "integer", default = 0, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(Vec3(0.5, 1.1, 0.5)),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	self:AddComponent("distribution", {
		inputs = { DistributionType.Gas },
		outputs = { DistributionType.Gas }
	})

	if CLIENT then
		self:SetInteractible(true)

		local model = AssetLibrary.GetModel("battery"):Clone()
		self.ChargeMaterial = model:GetMaterial(6):Clone()
		self.ChargeColor = Color(1.0, 0.0, 0.0)
		self.ChargeMaterial:SetValueProperty("EmissiveColor", self.ChargeColor * math.easeInCubic(0.0))

		model:SetMaterial(6, self.ChargeMaterial)

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	local gases = {[GasType.Oxygen] = "o2", [GasType.CarbonDioxyde] = "co2", [GasType.Nitrogen] = "n2"}
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")

		local capacity = self:GetProperty("capacity")

		local incomingGases = distribution:GetDistributedValue(DistributionType.Gas)

		local outputEntity = distribution:GetOutputConnectedEntity(0)
		local consumedGas
		if outputEntity then
			local outputDistribution = outputEntity:GetComponent("distribution")
			consumedGas = outputDistribution:GetConsumptionValue(distribution:GetOutputConnectedPort(0))
		else
			consumedGas = GasQuantity()
		end

		local consumptionValues = GasQuantity()
		for gasType, gasName in pairs(gases) do
			local incomingGas = math.min(incomingGases[gasType], maxInput)
			local outputValue = math.min(consumedGas[gasType], maxOutput)
			local storage = math.clamp(self:GetProperty("storage_" .. gasName) + incomingGas - outputValue, 0, capacity)
			self:UpdateProperty("storage_" .. gasName, storage)

			consumptionValues:Increment(gasType, math.min(capacity - storage, incomingGas))
		end

		distribution:UpdateConsumptionValue(0, consumptionValues)
	end)
else
	local function onStorageUpdate(self)
		local o2 = Distribution.FromStorageUnit(self:GetProperty("storage_o2"))
		local co2 = Distribution.FromStorageUnit(self:GetProperty("storage_co2"))
		local n2 = Distribution.FromStorageUnit(self:GetProperty("storage_n2"))

		local sum = math.max(o2 + co2 + n2, 1) -- avoid division by zero
		local o2_pct = math.floor(o2 * 100 / sum)
		local co2_pct = math.floor(co2 * 100 / sum)
		local n2_pct = math.floor(n2 * 100 / sum)

		self:SetInteractibleText(string.format("Gas storage\nOxygen: %.2fL (%d%%)\nCarbon dioxyde: %.2fL (%d%%)\nNitrogen: %.2fL (%d%%)\n", o2 / 1000, o2_pct, co2 / 1000, co2_pct, n2 / 1000, n2_pct))

		local capacity = self:GetProperty("capacity")
		local chargeFactor = sum / (capacity * 3)
		self.ChargeMaterial:SetValueProperty("EmissiveColor", self.ChargeColor * math.easeInCubic(chargeFactor))
	end

	classData:OnPropertyUpdate("storage_o2", onStorageUpdate)
	classData:OnPropertyUpdate("storage_co2", onStorageUpdate)
	classData:OnPropertyUpdate("storage_n2", onStorageUpdate)
end

EntityRegistry.RegisterClass("gas_storage", classData)
