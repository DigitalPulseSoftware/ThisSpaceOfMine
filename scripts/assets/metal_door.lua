local params = {
	mesh = {
	},
	loadMaterials = false
}

local model = Model.Load("CookedAssets/Models/MetalDoor/MetalDoor.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("AmbientOcclusionMap", Texture.Load("CookedAssets/Models/MetalDoor/Textures/AmbientOcclusion.dds"))
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/MetalDoor/Textures/BaseColor.dds"))
material:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/MetalDoor/Textures/MetallicRoughness.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/MetalDoor/Textures/Normal.dds"))
material:SetValueProperty("AlphaTest", true)

model:SetMaterial(0, material)

AssetLibrary.RegisterModel("metal_door", model)
