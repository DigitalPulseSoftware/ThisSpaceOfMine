local classData = EntityRegistry.ClassBuilder()

-- TODO: Replace by RPC
classData:AddProperty("sensor_o2", { type = "integer", default = 0, isNetworked = true })
classData:AddProperty("sensor_co2", { type = "integer", default = 0, isNetworked = true })
classData:AddProperty("sensor_n2", { type = "integer", default = 0, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "dynamic",
		mass = 10.0,
		collider = BoxCollider3D.new(Vec3f(0.75)),
		objectLayer = Constants.ObjectLayerDynamic
	}

	self:AddComponent("rigidbody3d", physSettings)

	if CLIENT then
		self:SetInteractible(true)
		self:SetInteractibleText("Sensor")

		local model = AssetLibrary.GetModel("computer")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end

	if SERVER then
		self:AllowEnvironmentSwitch()
		self:SetTickInterval(1000)
		self:AddComponent("atmosphere_monitor")
	end
end)

if SERVER then
	classData:On("tick", function (self)
		local monitor = self:GetComponent("atmosphere_monitor")
		if monitor.Atmosphere then
			self:UpdateProperty("sensor_o2", monitor.Atmosphere:GetGasAmount(GasType.Oxygen))
			self:UpdateProperty("sensor_co2", monitor.Atmosphere:GetGasAmount(GasType.CarbonDioxyde))
			self:UpdateProperty("sensor_n2", monitor.Atmosphere:GetGasAmount(GasType.Nitrogen))
		end
	end)
else
	local function onSensorUpdate(self)
		local o2 = self:GetProperty("sensor_o2")
		local co2 = self:GetProperty("sensor_co2")
		local n2 = self:GetProperty("sensor_n2")
		
		local sum = math.max(o2 + co2 + n2, 1) -- avoid division by zero
		local o2_pct = o2 * 100 // sum
		local co2_pct = co2 * 100 // sum
		local n2_pct = n2 * 100 // sum

		self:SetInteractibleText(string.format("Oxygen: %.2fL (%d%%)\nCarbon dioxyde: %.2fL (%d%%)\nNitrogen: %.2fL (%d%%)\n", o2 / 1000, o2_pct, co2 / 1000, co2_pct, n2 / 1000, n2_pct))
	end

	classData:OnPropertyUpdate("sensor_o2", onSensorUpdate)
	classData:OnPropertyUpdate("sensor_co2", onSensorUpdate)
	classData:OnPropertyUpdate("sensor_n2", onSensorUpdate)
end

EntityRegistry.RegisterClass("atmosphere_sensor", classData)
