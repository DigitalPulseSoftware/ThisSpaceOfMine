local params = {
	mesh = {
		center = true,
		--texCoordScale = Vec2(-1.0, 1.0),
		vertexRotation = EulerAngles(180, 0, 0),
		vertexScale = Vec3(1.0 / 500.0) * Vec3(1, -1, -1)
	},
	loadMaterials = false
}

local computer = Model.Load("assets/ship/computer/scifi_computer_1_3.obj", params)

local screenMat = MaterialInstance.Instantiate(MaterialType.Basic, MaterialInstancePreset.Transparent)
screenMat:SetTextureProperty("BaseColorMap", Texture.Load("assets/ship/computer/digital_displays.png"))

screenMat:UpdatePassesStates(function (renderStates)
	renderStates.faceCulling = FaceCulling.None
end)

local metalMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)

computer:SetMaterial(0, screenMat)
computer:SetMaterial(1, screenMat)
computer:SetMaterial(2, screenMat)
computer:SetMaterial(3, screenMat)
computer:SetMaterial(4, metalMat)
computer:SetMaterial(5, metalMat)

AssetLibrary.RegisterModel("computer", computer)
