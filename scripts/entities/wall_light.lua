local lightConsumption = Distribution.ToTickUnit(25)

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "wall_light")
classData:Set("spawnable_collider", Vec3(0.5, 0.1, 0.5))
classData:Set("spawnable_rotation", Vec3(90, 0, 0))

classData:AddProperty("light_enabled", { type = "bool", default = false, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(Vec3(0.5, 0.1, 0.5)),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	local distribution = self:AddComponent("distribution", {
		inputs = { DistributionType.Electrical },
		outputs = {}
	})

	if CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText("Light")

		local model = AssetLibrary.GetModel("wall_light"):Clone()
		self.LightMat = model:GetMaterial(2):Clone()
		model:SetMaterial(2, self.LightMat)

		local isEnabled = self:GetProperty("light_enabled")
		self.LightMat:SetValueProperty("EmissiveColor", isEnabled and Color.White or Color.Black)

		local light = self:AddComponent("light")
		light:AddSpotLight({ Color = Color.White, radius = 50 })
		light:Show(isEnabled)

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")
		local electricity = distribution:GetDistributedValue(DistributionType.Electrical)
		distribution:UpdateConsumptionValue(0, ElectricalQuantity(math.min(electricity.energy, lightConsumption)))

		self:UpdateProperty("light_enabled", electricity.energy >= lightConsumption)
	end)
else
	classData:OnPropertyUpdate("light_enabled", function (self, isEnabled)
		local light = self:GetComponent("light")
		light:Show(isEnabled)

		self.LightMat:SetValueProperty("EmissiveColor", isEnabled and Color.White or Color.Black)
	end)
end

EntityRegistry.RegisterClass("wall_light", classData)
