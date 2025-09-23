local classData = EntityRegistry.ClassBuilder()

classData:AddProperty("enabled", { type = "bool", default = true, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "dynamic",
		mass = 10.0,
		collider = BoxCollider3D.new(Vec3f(2.0, 0.1, 2.0)),
		objectLayer = Constants.ObjectLayerDynamic
	}

	self:AddComponent("rigidbody3d", physSettings)

	if SERVER then
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
	classData:OnPropertyUpdate("enabled", function (self)
		local text = "Solar panel"
		if not self:GetProperty("enabled") then
			text = text .. " (disabled)"
		end
		self:SetInteractibleText(text)
	end)
end

EntityRegistry.RegisterClass("solar_panel", classData)
