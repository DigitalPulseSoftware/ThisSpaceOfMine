local params = {
	mesh = {
		center = true,
        texCoordOffset = Vec2(0.0, 1.0),
        texCoordScale = Vec2(1.0, -1.0),
		--texCoordScale = Vec2(-1.0, 1.0),
		vertexRotation = EulerAngles(90, 0, 90),
		vertexScale = Vec3(1.0 / 10.0)
	},
	loadMaterials = false
}

local controlPanel = Model.Load("CookedAssets/Models/Sensor/Sensor.obj", params)

local material = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
material:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/Sensor/Textures/BaseColor.dds"))
material:SetTextureProperty("MetallicRoughnessMap", Texture.Load("CookedAssets/Models/Sensor/Textures/MetallicRoughness.dds"))
material:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/Sensor/Textures/Normal.dds"))

controlPanel:SetMaterial(0, material)

local ledMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
ledMat:SetValueProperty("EmissiveColor", Color.Cyan)

controlPanel:SetMaterial(1, ledMat)

local screenMaterial = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
screenMaterial:SetValueProperty("BaseColor", Color.Black)
screenMaterial:SetTextureProperty("EmissiveMap", Texture.Load("CookedAssets/Models/Sensor/Textures/Screen.dds"))

controlPanel:SetMaterial(2, screenMaterial)

AssetLibrary.RegisterModel("sensor", controlPanel)
