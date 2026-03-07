local CONVERSION_RATE = 10

local classData = EntityRegistry.ClassBuilder()

classData:AddProperty("scale", { type = "float", default = 1, isNetworked = true })

classData:On("init", function (self)
	if CLIENT then
		local model = AssetLibrary.GetModel("tree-01-" .. math.random(1, 4))

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)

		local node = self:GetComponent("node")
		node:Scale(Vec3f(self:GetProperty("scale")))
	end

	if SERVER then
		self:AddComponent("atmosphere_monitor")
		local exchanger = self:AddComponent("atmosphere_exchanger")
		exchanger:SetGasModifier(GasType.CarbonDioxyde, -CONVERSION_RATE)
		exchanger:SetGasModifier(GasType.Oxygen, CONVERSION_RATE)
	end
end)

EntityRegistry.RegisterClass("tree", classData)
