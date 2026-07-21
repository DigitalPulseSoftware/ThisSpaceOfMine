local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "light")
classData:Set("spawnable_collider", Vec3(0.5))

classData:AddProperty("light_enabled", { type = "bool", default = true, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "dynamic",
		mass = 1.0,
		collider = BoxCollider3D.new(Vec3(0.5)),
		objectLayer = Constants.ObjectLayerDynamic
	}

	self:AddComponent("rigidbody3d", physSettings)

	if SERVER then
		self:AllowEnvironmentSwitch()
		local distribution = self:AddComponent("distribution", {
			inputs = { DistributionType.Electrical },
			outputs = {}
		})
		distribution:UpdateConsumptionValue(0, 25)
	elseif CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText("Light")

		local model = AssetLibrary.GetModel("light")

		local light = self:AddComponent("light")
		light:AddPointLight()
		light:Show(self:GetProperty("light_enabled"))

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("tick", function (self)
		local distribution = self:GetComponent("distribution")
		local electricity = distribution:GetDistributedValue(DistributionType.Electrical)
		self:UpdateProperty("light_enabled", electricity > 0)
	end)
else
	classData:OnPropertyUpdate("light_enabled", function (self, isEnabled)
		local light = self:GetComponent("light")
		light:Show(isEnabled)
	end)
end
EntityRegistry.RegisterClass("light", classData)
