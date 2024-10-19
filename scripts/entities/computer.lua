local classData = EntityRegistry.ClassBuilder()

-- TODO: Replace by RPC
classData:AddProperty("active", { type = "bool", default = false, isNetworked = true })

classData:AddClientRPC("activate")

if CLIENT then
	classData:OnClientRPC("activate", function (self)
		ClientSession.EnableShipControl(true)
	end)
end

classData:On("init", function (self)
	local physSettings = {
		kind = "dynamic",
		mass = 0.0,
		geom = BoxCollider3D.new(Vec3(0.5)),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	self:SetInteractible(true)

	if CLIENT then
		self:SetInteractibleText("Pilot")

		local model = AssetLibrary.GetModel("computer")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("interact", function (self, player)
		local computerNode = self:GetComponent("node")
		local outsideEntity = self:GetEnvironment():GetOutsideShipEntity()

		player:GetController():SetShipController(ShipController.new(outsideEntity, computerNode:GetRotation()))
		self:CallClientRPC("activate", player)
	end)
end

EntityRegistry.RegisterClass("computer", classData)
