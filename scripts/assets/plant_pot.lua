local params = {
	mesh = {
		center = true,
		vertexRotation = EulerAngles(-90, 0, 0),
		vertexScale = Vec3(1.0 / 50.0)
	},
	loadMaterials = false
}

local plantPot = Model.Load("CookedAssets/Models/PlantPot/PlantPot.fbx", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/PlantPot/Textures/BaseColor.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/PlantPot/Textures/Normal.dds"))
material:SetTextureProperty("RoughnessMap", Texture.Load("CookedAssets/Models/PlantPot/Textures/Roughness.dds"))
material:SetValueProperty("Metallic", 0.0)

plantPot:SetMaterial(0, material)
plantPot:SetMaterial(1, material)

AssetLibrary.RegisterModel("plant_pot", plantPot)
