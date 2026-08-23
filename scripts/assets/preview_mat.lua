local previewMat = MaterialInstance.Instantiate(MaterialType.PhysicallyBased, MaterialInstancePresetFlags.AlphaBlended)
previewMat:SetValueProperty("BaseColor", Color(1.0, 1.0, 1.0, 0.5))

AssetLibrary.RegisterMaterialInstance("preview_mat", previewMat)
