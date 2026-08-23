local maxConsumption = Distribution.ToTickUnit(1000)
local modelSize = Vec3(0.816, 0.5, 1.8) * 0.25

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "electrical_hub")
classData:Set("spawnable_collider", modelSize)

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(modelSize),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	local distribution = self:AddComponent("distribution", {
		inputs = { DistributionType.Electrical, DistributionType.Electrical, DistributionType.Electrical, DistributionType.Electrical },
		outputs = { DistributionType.Electrical, DistributionType.Electrical, DistributionType.Electrical, DistributionType.Electrical }
	})

	if CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText("Electrical hub")

		local model = AssetLibrary.GetModel("electrical_hub")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")

		local inputCount = distribution:GetOutputCount()
		local outputCount = distribution:GetOutputCount()

		local connectedOutputCount = 0
		for i = 1, outputCount do
			if distribution:IsOutputConnected(i - 1) then
				connectedOutputCount = connectedOutputCount + 1 -- TODO: Use += 1
			end
		end

		local incomingPower = math.min(distribution:GetDistributedValue(DistributionType.Electrical).energy, maxConsumption)

		local perOutputProduction = math.floor(incomingPower / connectedOutputCount) -- TODO: Use //
		local productionOverflow = incomingPower - perOutputProduction * connectedOutputCount

		local totalConsumption = 0

		for i = 1, outputCount do
			local outputIndex = i - 1
			if distribution:IsOutputConnected(outputIndex) then
				-- Give overflow to the first connected output
				distribution:UpdateProductionValue(outputIndex, ElectricalQuantity(perOutputProduction + productionOverflow))

				local outputEntity = distribution:GetOutputConnectedEntity(outputIndex)
				local outputDistribution = outputEntity:GetComponent("distribution")
				local consumedValue = outputDistribution:GetConsumptionValue(distribution:GetOutputConnectedPort(outputIndex))

				totalConsumption = totalConsumption + consumedValue.energy
				productionOverflow = 0
			else
				distribution:UpdateProductionValue(outputIndex, ElectricalQuantity(0))
			end
		end

		-- Split consumption to all connected inputs
		local connectedInputCount = 0
		for i = 1, inputCount do
			if distribution:IsInputConnected(i - 1) then
				connectedInputCount = connectedInputCount + 1 -- TODO: Use += 1
			end
		end

		local perInputConsumption = math.floor(totalConsumption / connectedInputCount) -- TODO: Use //
		local consumptionOverflow = totalConsumption - perInputConsumption * connectedInputCount
		for i = 1, inputCount do
			local inputIndex = i - 1
			if distribution:IsInputConnected(inputIndex) then
				distribution:UpdateConsumptionValue(inputIndex, ElectricalQuantity(perInputConsumption + consumptionOverflow))

				consumptionOverflow = 0
			else
				distribution:UpdateConsumptionValue(inputIndex, ElectricalQuantity(0))
			end
		end
	end)
end

EntityRegistry.RegisterClass("electrical_hub", classData)
