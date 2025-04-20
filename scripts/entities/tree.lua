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
		self:SetTickInterval(1000)
		self:AddComponent("atmosphere_monitor")
	end
end)

if SERVER then
	classData:On("tick", function (self)
		local monitor = self:GetComponent("atmosphere_monitor")
		if monitor.Atmosphere then
			monitor.Atmosphere:Exchange({
				[GasType.CarbonDioxyde] = -CONVERSION_RATE,
				[GasType.Oxygen] = CONVERSION_RATE
			})
		end
	end)
end

EntityRegistry.RegisterClass("tree", classData)
