local params = {
	mesh = {
		center = true,
		vertexScale = Vec3(0.95)
	},
	loadMaterials = false
}

local lightSource = Model.Load("CookedAssets/Models/WallLights/WallLight1x1.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/WallLights/Textures/BaseColor.dds"))
material:SetTextureProperty("AmbientOcclusionMap", Texture.Load("CookedAssets/Models/WallLights/Textures/AmbientOcclusion.dds"))
material:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/WallLights/Textures/MetallicRoughness.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/WallLights/Textures/Normal.dds"))

lightSource:SetMaterial(0, material)
lightSource:SetMaterial(1, material)
lightSource:SetMaterial(2, material)

AssetLibrary.RegisterModel("wall_light", lightSource)
