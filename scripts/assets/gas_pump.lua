local params = {
	mesh = {
		center = true,
		vertexScale = Vec3(0.5)
	},
	loadMaterials = false
}

local model = Model.Load("CookedAssets/Models/GasPump/GasPump.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/GasPump/Textures/BaseColor.dds"))
material:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/GasPump/Textures/MetallicRoughness.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/GasPump/Textures/Normal.dds"))

model:SetMaterial(0, material)

local dialMaterial = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
dialMaterial:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/GasPump/Textures/Dial_BaseColor.dds"))

model:SetMaterial(1, dialMaterial)
model:SetMaterial(2, dialMaterial)

AssetLibrary.RegisterModel("gas_pump", model)
