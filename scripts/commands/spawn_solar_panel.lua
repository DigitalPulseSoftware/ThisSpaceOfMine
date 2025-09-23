return function ()
	local env = CurrentPlayer:GetEnvironment()
	local physWorld = env:GetPhysWorld()

    local controller = CurrentPlayer:GetController()

	local eyePos = controller:GetEyePosition()
	local cameraRot = controller:GetCameraRotation()

	local result = physWorld:RaycastQueryFirst(eyePos, eyePos + cameraRot * Vec3f(0, 0, -10), { IgnorePlayers = true })

	if not result.hitEntity or not result.hitChunk then
		print("no chunk hit")
		return
	end

    local solarPanel = env:CreateEntity("solar_panel", {
        position = result.hitPosition + result.hitNormal + Vec3f(-2, 0, 0)
    })

	local light = env:CreateEntity("light", {
        position = result.hitPosition + result.hitNormal + Vec3f(2, 0, 0)
    })

	local solarPanelDis = solarPanel:GetComponent("distribution")
	local solarPanelLight = light:GetComponent("distribution")

	solarPanelDis:ConnectOutput(0, light, 0)
	solarPanelLight:ConnectInput(0, solarPanel, 0)
end
