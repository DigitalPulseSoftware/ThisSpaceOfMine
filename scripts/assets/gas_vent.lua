local params = {
	mesh = {
		center = true,
		vertexRotation = EulerAngles(90, 0, 180),
		vertexScale = Vec3(1.0 / 4.0)
	},
	loadMaterials = false
}

local model = Model.Load("CookedAssets/Models/GasVent/GasVent.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/GasVent/Textures/BaseColor.dds"))
material:SetTextureProperty("AmbientOcclusionMap", Texture.Load("CookedAssets/Models/GasVent/Textures/AmbientOcclusion.dds"))
material:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/GasVent/Textures/MetallicRoughness.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/GasVent/Textures/Normal.dds"))

model:SetMaterial(0, material)

AssetLibrary.RegisterModel("gas_vent", model)
