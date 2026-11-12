return function ()
	local env = CurrentPlayer:GetEnvironment()

    local controller = CurrentPlayer:GetController()
	local eyePos = controller:GetEyePosition()

	local asteroid = env:CreateEntity("asteroid", {
		position = eyePos + controller:GetCameraRotation() * Vec3.Forward * 30,
		Seed = math.random(0, 1000),
		Radius = math.random() * 8 + 2
	})

	--[[local y = 0
	for ang = 0, 360, 10 do
		local rot = Quaternion.AngleAxis(0.5 * 3.1415, Vec3.Forward)

		local radAngle = math.rad(ang)
		local offsetX = math.sin(radAngle)
		local offsetZ = math.cos(radAngle)
		
		local radius = math.random() * 8 + 2
		local pos = Vec3(0, y, 0) + Vec3(offsetX, 0, offsetZ) * 200 * (1.0 - math.abs(y) / 250)
		--io.stderr:write("pos: " .. tostring(pos) .. "\n")

		local asteroid = env:CreateEntity("asteroid", {
			position = rot * pos,
			Seed = math.random(0, 1000),
			Radius = radius
		})

		asteroid:SetLifetime(120)

		local asteroidRigidBody = asteroid:GetComponent("rigidbody3d")
		asteroidRigidBody:SetLinearDamping(0)
		asteroidRigidBody:SetLinearVelocity(rot * Vec3(offsetZ, 0, -offsetX) * 60 * (1.0 - math.abs(y) / 250))
	end]]

	--]]

	--[[for i = 1, 100 do
		local offsetX = math.random() * 2.0 - 1.0
		local offsetY = math.random() * 2.0 - 1.0
		local offsetZ = math.random() * 2.0 - 1.0
		local radius = math.random() * 20 + 2

		local asteroid = env:CreateEntity("asteroid", {
			position = eyePos + Vec3(offsetX, offsetY, offsetZ) * 250,
			Seed = math.random(0, 2147483645),
			Radius = radius
		})

		local asteroidRigidBody = asteroid:GetComponent("rigidbody3d")
		asteroidRigidBody:SetAngularDamping(0)
		asteroidRigidBody:SetAngularDamping(Vec3(math.random() * 2.0 - 1.0, math.random() * 2.0 - 1.0, math.random() * 2.0 - 1.0) * math.random(0, 10))
		asteroidRigidBody:SetLinearDamping(0)
		asteroidRigidBody:SetLinearVelocity(Vec3(math.random() * 2.0 - 1.0, math.random() * 2.0 - 1.0, math.random() * 2.0 - 1.0) * math.random())
	end]]

end
