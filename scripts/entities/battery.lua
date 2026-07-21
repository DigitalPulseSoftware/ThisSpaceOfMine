local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "battery")
classData:Set("spawnable_collider", Vec3(0.5, 1.1, 0.5))

classData:AddProperty("capacity", { type = "integer", default = 10000 * 1000 / Constants.DistributionTickRate, isNetworked = true })
classData:AddProperty("charge", { type = "integer", default = 0, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(Vec3(0.5, 1.1, 0.5)),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	if SERVER then
		local distribution = self:AddComponent("distribution", {
			inputs = { DistributionType.Electrical },
			outputs = { DistributionType.Electrical }
		})
		distribution:UpdateConsumptionValue(0, 200)
		distribution:UpdateProductionValue(0, 0)
	elseif CLIENT then
		self:SetInteractible(true)

		local capacity = self:GetProperty("capacity")
		local charge = self:GetProperty("charge")

		local chargeFactor = charge / capacity
		self:SetInteractibleText(string.format("Battery (%d mAh - %d %%)", charge * Constants.DistributionTickRate / 1000, math.floor(chargeFactor * 100 + 0.5)))

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
		local incomingElectricity = distribution:GetDistributedValue(DistributionType.Electrical)

		local capacity = self:GetProperty("capacity")
		local charge = self:GetProperty("charge")

		if charge < capacity then
			distribution:UpdateConsumptionValue(0, math.min(capacity - charge, 200 * 1000 / Constants.DistributionTickRate))
		else
			distribution:UpdateConsumptionValue(0, 0)
		end

		local outputValue = math.min(charge, 200 * 1000 / Constants.DistributionTickRate)

		local outputEntity = distribution:GetOutputConnectedEntity(0)
		if outputEntity then
			local connectedSlot = distribution:GetOutputConnectedPort(0)
			local outputDistribution = outputEntity:GetComponent("distribution")
			local consumedValue = outputDistribution:GetConsumptionValue(connectedSlot)
			outputValue = math.min(outputValue, consumedValue)
		else
			outputValue = 0
		end

		distribution:UpdateProductionValue(0, outputValue)

		self:UpdateProperty("charge", math.min(charge + incomingElectricity - outputValue, capacity))
	end)
else
	classData:OnPropertyUpdate("charge", function (self, charge)
		local capacity = self:GetProperty("capacity")
		local chargeFactor = charge / capacity
		self:SetInteractibleText(string.format("Battery (%d mAh - %d %%)", math.floor(charge * Constants.DistributionTickRate / 1000), math.floor(chargeFactor * 100 + 0.5)))
		self.ChargeMaterial:SetValueProperty("EmissiveColor", self.ChargeColor * math.easeInCubic(chargeFactor))
	end)
end

EntityRegistry.RegisterClass("battery", classData)
