local modelSize = Vec3(1.0, 1.0, 1.0)
local conversionRate = 80

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "plant_pot")
classData:Set("spawnable_collider", modelSize)

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(modelSize),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	if SERVER then
		self:AddComponent("atmosphere_monitor")
		local exchanger = self:AddComponent("atmosphere_exchanger")
		exchanger:SetGasModifier(GasType.CarbonDioxyde, -conversionRate)
		exchanger:SetGasModifier(GasType.Oxygen, conversionRate)
	else
		self:SetInteractible(true)
		self:SetInteractibleText("Plant")

		local model = AssetLibrary.GetModel("plant_pot")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

EntityRegistry.RegisterClass("plant_pot", classData)
