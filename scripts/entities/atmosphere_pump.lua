local maxOutput = 1000
local modelSize = Vec3(1.09, 1.64, 1.92) * 0.5

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", false)
classData:Set("spawnable_model", "gas_pump")
classData:Set("spawnable_collider", modelSize)

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(modelSize),
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

		local model = AssetLibrary.GetModel("gas_pump")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end

	if SERVER then
		self:AddComponent("atmosphere_monitor")
	end
end)

if SERVER then
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")

		local monitor = self:GetComponent("atmosphere_monitor")
		if not monitor.Atmosphere then
			distribution:UpdateProductionValue(0, GasQuantity())
			return
		end

		local outputEntity = distribution:GetOutputConnectedEntity(0)
		if not outputEntity then
			return
		end

		local outputDistribution = outputEntity:GetComponent("distribution")
		local consumedGases = outputDistribution:GetConsumptionValue(distribution:GetOutputConnectedPort(0))

		local producedGases = GasQuantity()
		local gasTotalAmount = 0

		local exchangedGases = {}
		for gasName, gasType in pairs(GasType) do -- TODO: Iterate on GasQuantity
			local consumedQuantity = Distribution.FromTickUnit(consumedGases[gasType])
			local atmosphereQuantity = monitor.Atmosphere:GetGasAmount(gasType)
			if consumedQuantity > 0 then
				exchangedGases[gasType] = -math.min(atmosphereQuantity, consumedQuantity)

				-- Take new value into account for production
				atmosphereQuantity = math.max(atmosphereQuantity - consumedQuantity, 0)
			end

			producedGases[gasType] = atmosphereQuantity
			gasTotalAmount = gasTotalAmount + atmosphereQuantity
		end

		if not monitor.Atmosphere:Exchange(exchangedGases) then
			-- Shouldn't happen as we checked gas amount before exchanging
			error("failed to exchange gases")
		end

		-- Take % into account
		if gasTotalAmount > maxOutput then
			for gasName, gasType in pairs(GasType) do -- TODO: Iterate on GasQuantity
				producedGases[gasType] = Distribution.ToTickUnit(producedGases[gasType] * maxOutput / gasTotalAmount)
			end
		else
			for gasName, gasType in pairs(GasType) do -- TODO: Iterate on GasQuantity
				producedGases[gasType] = Distribution.ToTickUnit(producedGases[gasType])
			end
		end

		distribution:UpdateProductionValue(0, producedGases)
	end)
end

EntityRegistry.RegisterClass("atmosphere_pump", classData)
