local params = {
	mesh = {
		center = true,
	},
	loadMaterials = false
}

local model = Model.Load("CookedAssets/Models/SolarPanel/SolarPanel.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("AmbientOcclusionMap", Texture.Load("CookedAssets/Models/SolarPanel/Textures/AmbientOcclusion.dds"))
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/SolarPanel/Textures/BaseColor.dds"))
material:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/SolarPanel/Textures/MetallicRoughness.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/SolarPanel/Textures/Normal.dds"))
material:SetValueProperty("ShadowMapNormalOffset", -0.02)

model:SetMaterial(0, material)

local metalMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
metalMat:SetValueProperty("ShadowMapNormalOffset", -0.02)
metalMat:SetValueProperty("MetallicFactor", 1.0)
metalMat:SetValueProperty("RoughnessFactor", 0.2)

model:SetMaterial(1, metalMat)
model:SetMaterial(2, metalMat)

local plasticMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
plasticMat:SetValueProperty("BaseColor", Color.Black)
model:SetMaterial(3, plasticMat)

AssetLibrary.RegisterModel("solar_panel", model)
