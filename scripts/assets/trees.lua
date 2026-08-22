local barkMaterial = MaterialInstance.Instantiate(MaterialType.PhysicallyBased)
barkMaterial:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/Trees/Textures/Bark_BaseColor.dds"))
barkMaterial:SetTextureSamplerProperty("BaseColorMap", { wrapModeU = SamplerWrap.Repeat, wrapModeV = SamplerWrap.Repeat })
barkMaterial:SetTextureProperty("NormalMap", Texture.Load("CookedAssets/Models/Trees/Textures/Bark_Normal.dds"))
barkMaterial:SetTextureSamplerProperty("NormalMap", { wrapModeU = SamplerWrap.Repeat, wrapModeV = SamplerWrap.Repeat })
barkMaterial:SetValueProperty("MetallicFactor", 0.0)
barkMaterial:SetValueProperty("RoughnessFactor", 1.0)

local leavesMaterial = AssetLibrary.GetMaterial("LeafPBRMaterial"):Instantiate()
leavesMaterial:SetTextureProperty("AlphaMap", Texture.Load("CookedAssets/Models/Trees/Textures/Leaves_BaseColor.dds"))
leavesMaterial:SetValueProperty("AlphaTest", true)
leavesMaterial:SetValueProperty("MetallicFactor", 0.0)
leavesMaterial:SetValueProperty("RoughnessFactor", 1.0)
leavesMaterial:SetValueProperty("ShadowMapNormalOffset", 0.1)

leavesMaterial:UpdatePassesStates(function (renderStates)
	renderStates.faceCulling = FaceCulling.None
end)

local meshParams = {
	--center = true,
	--texCoordScale = Vec2(-1.0, 1.0),
	vertexRotation = EulerAngles(-90, 0, 0),
	--vertexScale = Vec3(1.0 / 4.0)
}

for i = 1, 4 do
	local treeMesh = Mesh.Load("CookedAssets/Models/Trees/Tree" .. i .. ".obj", meshParams)
	treeMesh:GetSubMesh(1):GenerateTangents()

    local treeModel = Model.BuildFromMesh(treeMesh)
    treeModel:SetMaterial(0, barkMaterial)
    treeModel:SetMaterial(1, leavesMaterial)

    AssetLibrary.RegisterModel("tree" .. i, treeModel)
end
