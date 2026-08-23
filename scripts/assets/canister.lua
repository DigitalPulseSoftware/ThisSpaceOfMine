local params = {
	mesh = {
		center = true,
		vertexScale = Vec3(1.0 / 2.0)
	},
	loadMaterials = false
}

local model = Model.Load("CookedAssets/Models/Canister/Canister.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/Canister/Textures/BaseColor.dds"))
material:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/Canister/Textures/MetallicRoughness.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/Canister/Textures/Normal.dds"))

model:SetMaterial(0, material)

AssetLibrary.RegisterModel("canister", model)
