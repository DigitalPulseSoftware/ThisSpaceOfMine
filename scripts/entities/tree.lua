local CONVERSION_RATE = 10

local classData = EntityRegistry.ClassBuilder()
classData:AddProperty("color", { type = "float3", default = Vec3.Zero, isNetworked = true})
classData:AddProperty("scale", { type = "float", default = 1, isNetworked = true })
classData:AddProperty("variation", { type = "integer", default = 0, isNetworked = true })

classData:On("init", function (self)
	if SERVER then
		if self:GetProperty("color") == Vec3.Zero then
			self:UpdateProperty("color", Vec3(0.622, 1.0, 0.075) * (0.3 + math.random() * 0.3))
		end
		if self:GetProperty("variation") <= 0 then
			self:UpdateProperty("variation", math.random(1, 4))
		end

		self:AddComponent("atmosphere_monitor")
		local exchanger = self:AddComponent("atmosphere_exchanger")
		exchanger:SetGasModifier(GasType.CarbonDioxyde, -CONVERSION_RATE)
		exchanger:SetGasModifier(GasType.Oxygen, CONVERSION_RATE)
	else
		local leafColor = self:GetProperty("color")

		local model = AssetLibrary.GetModel("tree" .. self:GetProperty("variation")):Clone()
		self.LeafMaterial = model:GetMaterial(1):Clone()
		self.LeafMaterial:SetValueProperty("BaseColor", Color(leafColor.x, leafColor.y, leafColor.z))
		model:SetMaterial(1, self.LeafMaterial)

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)

		local node = self:GetComponent("node")
		node:Scale(Vec3(self:GetProperty("scale")))
	end
end)

EntityRegistry.RegisterClass("tree", classData)
