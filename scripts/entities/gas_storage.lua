local maxOutput = Distribution.ToTickUnit(200)
local modelSize = Vec3(1.0, 1.58, 1.0)

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", false)
classData:Set("spawnable_model", "canister")
classData:Set("spawnable_collider", modelSize)

classData:AddProperty("capacity", { type = "integer", default = Distribution.ToStorageUnit(10000), isNetworked = true })

local orderedGasTypes = {}
for gasName, gasType in pairs(GasType) do
    table.insert(orderedGasTypes, gasName)
end
table.sort(orderedGasTypes)

-- TODO: Replace by RPC
for _, gasName in pairs(orderedGasTypes) do
	classData:AddProperty("storage_" .. gasName, { type = "integer", default = 0, isNetworked = true })
end

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(modelSize),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	self:AddComponent("distribution", {
		inputs = { DistributionType.Gas },
		outputs = { DistributionType.Gas }
	})

	if CLIENT then
		self:SetInteractible(true)

		local model = AssetLibrary.GetModel("canister")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")

		local capacity = self:GetProperty("capacity")

		local incomingGases = distribution:GetDistributedValue(DistributionType.Gas)

		local outputEntity = distribution:GetOutputConnectedEntity(0)
		local consumedGases
		if outputEntity then
			local outputDistribution = outputEntity:GetComponent("distribution")
			consumedGases = outputDistribution:GetConsumptionValue(distribution:GetOutputConnectedPort(0))
		else
			consumedGases = GasQuantity()
		end

		local totalIncomingGas = 0
		local totalStoredGas = 0
		for gasName, gasType in pairs(GasType) do -- TODO: Iterate on GasQuantity
			totalIncomingGas = totalIncomingGas + incomingGases[gasType]
			totalStoredGas = totalStoredGas + self:GetProperty("storage_" .. gasName)
		end

		local remainingCapacity = self:GetProperty("capacity") - totalStoredGas
		if totalIncomingGas > remainingCapacity then
			for gasName, gasType in pairs(GasType) do -- TODO: Iterate on GasQuantity
				local incomingGas = incomingGases[gasType]
				incomingGases[gasType] = math.min(incomingGas, incomingGas / totalIncomingGas * remainingCapacity)
			end
		end

		local consumptionValues = GasQuantity()
		for gasName, gasType in pairs(GasType) do
			local incomingGas = incomingGases[gasType]
			local outputValue = 0--math.min(consumedGases[gasType], maxOutput)
			local storage = math.max(self:GetProperty("storage_" .. gasName) + incomingGas - outputValue, 0)
			self:UpdateProperty("storage_" .. gasName, storage)

			consumptionValues[gasType] = incomingGas
		end

		distribution:UpdateConsumptionValue(0, consumptionValues)
	end)
else
	local function onStorageUpdate(self)
		local sum = 0
		for gasName, gasType in pairs(GasType) do
			sum = sum + Distribution.FromStorageUnit(self:GetProperty("storage_" .. gasName))
		end

		local capacity = Distribution.FromStorageUnit(self:GetProperty("capacity"))

		local text = {string.format("Gas storage (%.2f/%.2fg)", sum / 1000, capacity / 1000)}
		for _, gasName in pairs(orderedGasTypes) do
			local quantity = Distribution.FromStorageUnit(self:GetProperty("storage_" .. gasName))
			if quantity > 0 then
				table.insert(text, string.format("%s: %.2fg (%d%%)", gasName, quantity / 1000, quantity / sum * 100))
			end
		end

		self:SetInteractibleText(table.concat(text, "\n"))
	end

	for gasName, gasType in pairs(GasType) do
		classData:OnPropertyUpdate("storage_" .. gasName, onStorageUpdate)
	end
end

EntityRegistry.RegisterClass("gas_storage", classData)
