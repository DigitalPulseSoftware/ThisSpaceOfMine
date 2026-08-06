local maxOutputValue = Distribution.ToTickUnit(1000)

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "switch")
classData:Set("spawnable_collider", Vec3(0.25))

classData:AddProperty("enabled", { type = "bool", default = true, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(Vec3(0.25)),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)
	self:SetInteractible(true)

	self:AddComponent("distribution", {
		inputs = { DistributionType.Electrical },
		outputs = { DistributionType.Electrical }
	})

	if CLIENT then
		self:SetInteractibleText("Switch" .. (self:GetProperty("enabled") and " (enabled)" or " (disabled)"))

		local model = AssetLibrary.GetModel("switch")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("interact", function (self, player)
		self:UpdateProperty("enabled", not self:GetProperty("enabled"))
		if not self:GetProperty("enabled") then
			-- TODO: Disable tick when not enabled
			local distribution = self:GetComponent("distribution")
			distribution:UpdateConsumptionValue(0, ElectricalQuantity(0))
			distribution:UpdateProductionValue(0, ElectricalQuantity(0))
		end
	end)

	classData:On("distribution", function (self)
		if not self:GetProperty("enabled") then
			return
		end

		local distribution = self:GetComponent("distribution")

		local electricity = distribution:GetDistributedValue(DistributionType.Electrical).energy
		distribution:UpdateProductionValue(0, ElectricalQuantity(math.min(electricity, maxOutputValue)))

		local outputEntity = distribution:GetOutputConnectedEntity(0)
		if outputEntity then
			local outputDistribution = outputEntity:GetComponent("distribution")
			distribution:UpdateConsumptionValue(0, outputDistribution:GetConsumptionValue(0))
		else
			distribution:UpdateConsumptionValue(0, ElectricalQuantity(0))
		end
	end)
else
	classData:OnPropertyUpdate("enabled", function (self, isEnabled)
		self:SetInteractibleText(isEnabled and "Switch (enabled)" or "Switch (disabled)")
	end)
end
EntityRegistry.RegisterClass("switch", classData)
