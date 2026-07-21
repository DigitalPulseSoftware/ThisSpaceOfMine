local primitive = Primitive.Box(Vec3(2.0, 0.1, 2.0))

local mesh = Mesh.CreateStatic()
mesh:BuildSubMesh(primitive)
mesh:SetMaterialCount(1)

local model = Model.BuildFromMesh(mesh)

local solarPanelMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
solarPanelMat:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Textures/Dev/grey.dds"))
solarPanelMat:SetValueProperty("BaseColor", Color(0.2, 0.2, 1.0))

model:SetMaterial(0, solarPanelMat)

AssetLibrary.RegisterModel("solar_panel", model)
