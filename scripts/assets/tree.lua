local allTreeMesh = Mesh.Load("assets/tree/shapespark-low-poly-plants-kit.gltf")

local trees = {
    ["tree-01-1"] = {
        submeshes = {0,1,2}
    },
    ["tree-01-2"] = {
        submeshes = {3,4,5}
    },
    ["tree-01-3"] = {
        submeshes = {6,7,8}
    },
    ["tree-01-4"] = {
        submeshes = {9,10,11}
    }
}

for treeName, treeData in pairs(trees) do
    local treeMesh = Mesh.CreateStatic()
    treeMesh:SetMaterialCount(#treeData.submeshes)
    for idx, submeshIndex in ipairs(treeData.submeshes) do
        local submesh = allTreeMesh:GetSubMesh(submeshIndex)
        submesh:SetMaterialIndex(idx - 1)

        treeMesh:AddSubMesh(submesh)
        treeMesh:SetMaterialData(idx - 1, allTreeMesh:GetMaterialData(submesh:GetMaterialIndex()))
    end

    local aabb = treeMesh:GetAABB()
    treeMesh:Translate(-aabb:GetCenter() + Vec3f(0, aabb.width * 0.5, 0))

    local treeModel = Model.BuildFromMesh(treeMesh)
    AssetLibrary.RegisterModel(treeName, treeModel)
end

