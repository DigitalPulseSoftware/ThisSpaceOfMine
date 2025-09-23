local primitive = Primitive.Box(Vec3f(0.5, 0.5, 0.5))

local mesh = Mesh.CreateStatic()
mesh:BuildSubMesh(primitive)
mesh:SetMaterialCount(1)

local model = Model.BuildFromMesh(mesh)

local lightMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
lightMat:SetTextureProperty("BaseColorMap", Texture.Load("assets/dev/grey.png"))

model:SetMaterial(0, lightMat)

AssetLibrary.RegisterModel("light", model)
