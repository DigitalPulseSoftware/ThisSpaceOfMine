local modelSize = Vec3(1.72, 0.69, 1.72) / 4.0

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", false)
classData:Set("spawnable_model", "gas_vent")
classData:Set("spawnable_collider", modelSize)

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(modelSize),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	local distribution = self:AddComponent("distribution", {
		inputs = { DistributionType.Gas },
		outputs = {}
	})

	if CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText("Gas vent")

		local model = AssetLibrary.GetModel("gas_vent")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")
	end)
end

EntityRegistry.RegisterClass("atmosphere_vent", classData)
