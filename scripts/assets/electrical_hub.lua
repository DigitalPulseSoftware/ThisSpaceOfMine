local primitive = Primitive.Box(Vec3(0.5, 0.2, 0.5))

local mesh = Mesh.CreateStatic()
mesh:BuildSubMesh(primitive)
mesh:SetMaterialCount(1)

local model = Model.BuildFromMesh(mesh)

local lightMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
lightMat:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Textures/Dev/grey.dds"))

model:SetMaterial(0, lightMat)

AssetLibrary.RegisterModel("electrical_hub", model)
