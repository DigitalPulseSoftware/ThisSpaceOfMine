local modelSize = Vec3(1.49, 0.466, 4.07) / 10.0

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "sensor")
classData:Set("spawnable_collider", modelSize)

local orderedGasTypes = {}
for gasName, gasType in pairs(GasType) do
    table.insert(orderedGasTypes, gasName)
end
table.sort(orderedGasTypes)

-- TODO: Replace by RPC
for _, gasName in pairs(orderedGasTypes) do
	classData:AddProperty("sensor_" .. gasName, { type = "integer", default = 0, isNetworked = true })
end

local collider = BoxCollider3D.new(modelSize)

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = collider,
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	if CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText("Sensor")

		local model = AssetLibrary.GetModel("sensor")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end

	if SERVER then
		self:SetTickInterval(1000)
		self:AddComponent("atmosphere_monitor")
	else
		self:UpdateInfo()
	end
end)

if SERVER then
	classData:On("tick", function (self)
		local monitor = self:GetComponent("atmosphere_monitor")
		if monitor.Atmosphere then
			for gasName, gasType in pairs(GasType) do
				self:UpdateProperty("sensor_" .. gasName, monitor.Atmosphere:GetGasAmount(gasType))
			end
		else
			for gasName, gasType in pairs(GasType) do
				self:UpdateProperty("sensor_" .. gasName, 0)
			end
		end
	end)
else
	function classData:UpdateInfo()
		local sum = 0
		for gasName, gasType in pairs(GasType) do
			sum = sum + self:GetProperty("sensor_" .. gasName)
		end

		local text = {}
		for _, gasName in pairs(orderedGasTypes) do
			local quantity = self:GetProperty("sensor_" .. gasName)
			if quantity > 0 then
				table.insert(text, string.format("%s: %.2fg (%d%%)", gasName, quantity / 1000, quantity / sum * 100))
			end
		end

		if #text > 0 then
			self:SetInteractibleText(table.concat(text, "\n"))
		else
			self:SetInteractibleText("No atmosphere")
		end
	end

	for gasName, gasType in pairs(GasType) do
		classData:OnPropertyUpdate("sensor_" .. gasName, function (self) self:UpdateInfo() end)
	end
end

EntityRegistry.RegisterClass("atmosphere_sensor", classData)
