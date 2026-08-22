local params = {
	mesh = {
		center = true,
		vertexRotation = EulerAngles(90, 0, 90),
		vertexScale = Vec3(1.0 / 2.0)
	},
	loadMaterials = false
}

local model = Model.Load("CookedAssets/Models/SwitchBox/SwitchBox.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/SwitchBox/Textures/BaseColor.dds"))
material:SetTextureProperty("AmbientOcclusionMap", Texture.Load("CookedAssets/Models/SwitchBox/Textures/AmbientOcclusion.dds"))
material:SetTextureProperty("MetallicMap", Texture.Load("CookedAssets/Models/SwitchBox/Textures/Metallic.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/SwitchBox/Textures/Normal.dds"))

model:SetMaterial(0, material)
model:SetMaterial(1, material)

AssetLibrary.RegisterModel("switch", model)
