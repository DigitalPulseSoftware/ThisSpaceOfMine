local primitive = Primitive.Box(Vec3(0.25, 0.25, 0.25))

local mesh = Mesh.CreateStatic()
mesh:BuildSubMesh(primitive)
mesh:SetMaterialCount(1)

local model = Model.BuildFromMesh(mesh)

local switchMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
switchMat:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Textures/Dev/grey.dds"))

model:SetMaterial(0, switchMat)

AssetLibrary.RegisterModel("switch", model)
