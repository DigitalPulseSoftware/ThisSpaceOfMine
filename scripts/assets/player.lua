local playerMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
playerMat:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/Player/Textures/Soldier_AlbedoTransparency.dds"))
playerMat:SetTextureProperty("AmbientOcclusionMap", Texture.Load("CookedAssets/Models/Player/Textures/Soldier_AO.dds"))
playerMat:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/Player/Textures/Soldier_MetallicRoughness.dds"))
playerMat:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/Player/Textures/Soldier_Normal.dds"))

AssetLibrary.RegisterMaterialInstance("PlayerMaterial", playerMat)

-- TODO: Move player mesh loading here
