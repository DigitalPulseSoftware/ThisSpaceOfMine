local params = {
	mesh = {
		center = true,
		vertexScale = Vec3(0.2)
	},
	loadMaterials = false
}

local battery = Model.Load("CookedAssets/Models/Battery/Battery.obj", params)

local mat1 = MaterialInstance.Instantiate(MaterialType.PhysicallyBased, MaterialInstancePresetFlags.AlphaBlended)
mat1:SetValueProperty("BaseColor", Color(0, 0, 0, 0.5))

mat1:UpdatePassesStates(function (renderStates)
	renderStates.faceCulling = FaceCulling.None
end)

local mat2 = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
mat2:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/Battery/Textures/lambert3SG_baseColor.dds"))
mat2:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/Battery/Textures/lambert3SG_normal.dds"))

local mat3 = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
mat3:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/Battery/Textures/lambert4SG_baseColor.dds"))

local mat4 = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
mat4:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/Battery/Textures/lambert5SG_baseColor.dds"))
mat4:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/Battery/Textures/lambert5SG_normal.dds"))

local mat5 = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
mat5:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/Battery/Textures/lambert6SG_baseColor.dds"))
mat5:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/Battery/Textures/lambert6SG_normal.dds"))

local mat6 = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
mat6:SetValueProperty("BaseColor", Color(0.0377317034, 0.0312860906, 0.0312860906))

local mat7 = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
mat7:SetValueProperty("EmissiveColor", Color(0.106969856, 0.891318083, 0.0688068941))

battery:SetMaterial(0, mat1)
battery:SetMaterial(1, mat2)
battery:SetMaterial(2, mat3)
battery:SetMaterial(3, mat4)
battery:SetMaterial(4, mat5)
battery:SetMaterial(5, mat6)
battery:SetMaterial(6, mat7)

AssetLibrary.RegisterModel("battery", battery)
