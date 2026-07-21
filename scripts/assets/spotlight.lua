local primitive = Primitive.Cone(0.25, 0.5)

local mesh = Mesh.CreateStatic()
mesh:BuildSubMesh(primitive, { vertexRotation = EulerAngles(90, 0, 0) })
mesh:SetMaterialCount(1)

local model = Model.BuildFromMesh(mesh)

local lightMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
lightMat:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Textures/Dev/grey.dds"))

model:SetMaterial(0, lightMat)

AssetLibrary.RegisterModel("spotlight", model)
