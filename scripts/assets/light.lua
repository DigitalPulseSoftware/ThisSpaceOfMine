local params = {
	mesh = {
		center = true
	},
	loadMaterials = false
}

local lightSource = Model.Load("CookedAssets/Models/LightSource/LightSource.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/LightSource/Textures/BaseColor.dds"))
material:SetTextureProperty("AmbientOcclusionMap", Texture.Load("CookedAssets/Models/LightSource/Textures/AmbientOcclusion.dds"))
material:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/LightSource/Textures/MetallicRoughness.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/LightSource/Textures/Normal.dds"))

lightSource:SetMaterial(0, material)
lightSource:SetMaterial(1, material)

AssetLibrary.RegisterModel("light", lightSource)
