local maxInput = Distribution.ToTickUnit(200)
local maxOutput = Distribution.ToTickUnit(200)
local modelSize = Vec3(3.69, 5.56, 3.37) * 0.2

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "battery")
classData:Set("spawnable_collider", modelSize)

classData:AddProperty("capacity", { type = "integer", default = Distribution.ToStorageUnit(10000), isNetworked = true })
classData:AddProperty("charge", { type = "integer", default = 0, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(modelSize),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	local distribution = self:AddComponent("distribution", {
		inputs = { DistributionType.Electrical },
		outputs = { DistributionType.Electrical }
	})

	if SERVER then
		distribution:UpdateConsumptionValue(0, ElectricalQuantity(maxOutput))
		distribution:UpdateProductionValue(0, ElectricalQuantity(0))
	elseif CLIENT then
		self:SetInteractible(true)

		local capacity = self:GetProperty("capacity")
		local charge = self:GetProperty("charge")

		local chargeFactor = charge / capacity
		self:SetInteractibleText(string.format("Battery (%d Wh - %d %%)", Distribution.FromStorageUnit(charge), math.floor(chargeFactor * 100 + 0.5)))

		local model = AssetLibrary.GetModel("battery"):Clone()
		self.ChargeMaterial = model:GetMaterial(6):Clone()
		self.ChargeColor = self.ChargeMaterial:GetValueProperty("EmissiveColor")
		self.ChargeMaterial:SetValueProperty("EmissiveColor", self.ChargeColor * math.easeInCubic(chargeFactor))

		model:SetMaterial(6, self.ChargeMaterial)

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")

		local capacity = self:GetProperty("capacity")

		local incomingElectricity = math.min(distribution:GetDistributedValue(DistributionType.Electrical).energy, maxInput)

		local outputEntity = distribution:GetOutputConnectedEntity(0)
		local outputValue = 0
		if outputEntity then
			local outputDistribution = outputEntity:GetComponent("distribution")
			local consumedValue = outputDistribution:GetConsumptionValue(distribution:GetOutputConnectedPort(0)).energy
			outputValue = math.min(maxOutput, consumedValue)
		end

		local charge = math.clamp(self:GetProperty("charge") + incomingElectricity - outputValue, 0, capacity)
		self:UpdateProperty("charge", charge)

		local outputValue = math.min(charge, maxOutput)
		distribution:UpdateConsumptionValue(0, ElectricalQuantity(math.min(capacity - charge, incomingElectricity)))
		distribution:UpdateProductionValue(0, ElectricalQuantity(outputValue))
	end)
else
	classData:OnPropertyUpdate("charge", function (self, charge)
		local capacity = self:GetProperty("capacity")
		local chargeFactor = charge / capacity
		self:SetInteractibleText(string.format("Battery (%d Wh - %d %%)", math.floor(Distribution.FromStorageUnit(charge)), math.floor(chargeFactor * 100 + 0.5)))
		self.ChargeMaterial:SetValueProperty("EmissiveColor", self.ChargeColor * math.easeInCubic(chargeFactor))
	end)
end

EntityRegistry.RegisterClass("battery", classData)
