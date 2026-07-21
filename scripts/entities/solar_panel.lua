local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "solar_panel")
classData:Set("spawnable_collider", Vec3(2.0, 0.1, 2.0))

classData:AddProperty("enabled", { type = "bool", default = true, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "dynamic",
		mass = 10.0,
		collider = BoxCollider3D.new(Vec3(2.0, 0.1, 2.0)),
		objectLayer = Constants.ObjectLayerDynamic
	}

	self:AddComponent("rigidbody3d", physSettings)

	if SERVER then
		self:AllowEnvironmentSwitch()
		local distribution = self:AddComponent("distribution", {
			inputs = {}, 
			outputs = { DistributionType.Electrical }
		})
		distribution:UpdateProductionValue(0, 100)
		self:SetInteractible(true)
	elseif CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText("Solar panel")

		local model = AssetLibrary.GetModel("solar_panel")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("interact", function (self, player)
		self:UpdateProperty("enabled", not self:GetProperty("enabled"))
		local distribution = self:GetComponent("distribution")
		distribution:UpdateProductionValue(0, self:GetProperty("enabled") and 100 or 0)
	end)
else
	classData:OnPropertyUpdate("enabled", function (self, isEnabled)
		local text = "Solar panel"
		if not isEnabled then
			text = text .. " (disabled)"
		end
		self:SetInteractibleText(text)
	end)
end

EntityRegistry.RegisterClass("solar_panel", classData)
