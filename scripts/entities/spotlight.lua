local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "spotlight")
classData:Set("spawnable_collider", Vec3(0.5))

classData:AddProperty("light_enabled", { type = "bool", default = false, isNetworked = true })
classData:AddProperty("consumption", { type = "integer", default = 25, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(Vec3(0.5)),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	if SERVER then
		local distribution = self:AddComponent("distribution", {
			inputs = { DistributionType.Electrical },
			outputs = {}
		})
		distribution:UpdateConsumptionValue(0, Distribution.ToTickUnit(self:GetProperty("consumption")))
	elseif CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText("Light")

		local model = AssetLibrary.GetModel("spotlight"):Clone()
		self.LightMat = model:GetMaterial(0):Clone()
		model:SetMaterial(0, self.LightMat)

		local light = self:AddComponent("light")
		light:AddSpotLight({ Color = Color.White, radius = 50 })
		light:Show(self:GetProperty("light_enabled"))

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")
		local electricity = distribution:GetDistributedValue(DistributionType.Electrical)
		self:UpdateProperty("light_enabled", electricity >= Distribution.ToTickUnit(self:GetProperty("consumption")))
	end)
else
	classData:OnPropertyUpdate("light_enabled", function (self, isEnabled)
		local light = self:GetComponent("light")
		light:Show(isEnabled)

		self.LightMat:UpdateValueProperty("EmissiveColor", isEnabled and Color.White or Color.Black)
	end)
end
EntityRegistry.RegisterClass("spotlight", classData)
