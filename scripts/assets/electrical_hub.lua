local params = {
	mesh = {
		center = true,
		--texCoordScale = Vec2(-1.0, 1.0),
		vertexRotation = EulerAngles(90, 0, 90),
		vertexScale = Vec3(1.0 / 4.0)
	},
	loadMaterials = false
}

local controlPanel = Model.Load("CookedAssets/Models/ControlPanel/ControlPanel.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/ControlPanel/Textures/BaseColor.dds"))
material:SetTextureProperty("AmbientOcclusionMap", Texture.Load("CookedAssets/Models/ControlPanel/Textures/AmbientOcclusion.dds"))
material:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/ControlPanel/Textures/MetallicRoughness.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/ControlPanel/Textures/Normal.dds"))

controlPanel:SetMaterial(0, material)

AssetLibrary.RegisterModel("electrical_hub", controlPanel)
