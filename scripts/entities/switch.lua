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

	if SERVER then
		local distribution = self:AddComponent("distribution", {
			inputs = { DistributionType.Electrical },
			outputs = { DistributionType.Electrical }
		})
		distribution:UpdateConsumptionValue(0, Distribution.ToTickUnit(1000))
	elseif CLIENT then
		self:SetInteractibleText("Switch (enabled)")

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
			distribution:UpdateConsumptionValue(0, 0)
			distribution:UpdateProductionValue(0, 0)
		end
	end)

	classData:On("distribution", function (self)
		if not self:GetProperty("enabled") then
			return
		end

		local distribution = self:GetComponent("distribution")

		local electricity = distribution:GetDistributedValue(DistributionType.Electrical)
		distribution:UpdateProductionValue(0, electricity)

		local outputEntity = distribution:GetOutputConnectedEntity(0)
		if outputEntity then
			local outputDistribution = outputEntity:GetComponent("distribution")
			distribution:UpdateConsumptionValue(0, outputDistribution:GetConsumptionValue(0))
		else
			distribution:UpdateConsumptionValue(0, electricity)
		end
	end)
else
	classData:OnPropertyUpdate("enabled", function (self, isEnabled)
		self:SetInteractibleText(isEnabled and "Switch (enabled)" or "Switch (disabled)")
	end)
end
EntityRegistry.RegisterClass("switch", classData)
