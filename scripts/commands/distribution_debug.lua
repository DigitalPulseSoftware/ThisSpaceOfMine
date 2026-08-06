return function (opt)
	local env = CurrentPlayer:GetEnvironment()
	local physWorld = env:GetPhysWorld()

    local controller = CurrentPlayer:GetController()

	local eyePos = controller:GetEyePosition()
	local cameraRot = controller:GetCameraRotation()

	local result = physWorld:RaycastQueryFirst(eyePos, eyePos + cameraRot * Vec3(0, 0, -10), { IgnorePlayers = true })
	if not result.hitEntity then
		return
	end

	local distribution = result.hitEntity:GetComponent("distribution")
	assert(distribution, "Entity has no distribution component")

	local inputCount = distribution:GetInputCount()
	local outputCount = distribution:GetOutputCount()

	print("Electrical distributed value: " .. Distribution.FromTickUnit(distribution:GetDistributedValue(DistributionType.Electrical).energy))

	print(string.format("%d input slots:", inputCount))
	for i = 1, inputCount do
		local inputIndex = i - 1
		--print("Input type: " .. distribution:GetInputType(inputIndex))
		local portStr = "- Input port #" .. inputIndex

		local connectedEntity = distribution:GetInputConnectedEntity(inputIndex)
		if connectedEntity then
			portStr = portStr .. " (connected to port " .. distribution:GetInputConnectedPort(inputIndex) .. ")"
		else
			portStr = portStr .. " (not connected)"
		end

		print(portStr)
		print("  - consumption value: " .. Distribution.FromTickUnit(distribution:GetConsumptionValue(inputIndex).energy))
	end
	
	print(string.format("%d output slots:", inputCount))
	for i = 1, outputCount do
		local outputIndex = i - 1
		--print("Output type: " .. distribution:GetInputType(outputIndex))
		local portStr = "- Output port #" .. outputIndex

		local connectedEntity = distribution:GetOutputConnectedEntity(outputIndex)
		if connectedEntity then
			portStr = portStr .. " (connected to port " .. distribution:GetOutputConnectedPort(outputIndex) .. ")"
		else
			portStr = portStr .. " (not connected)"
		end

		print(portStr)
		print("  - production value: " .. Distribution.FromTickUnit(distribution:GetProductionValue(outputIndex).energy))
	end
end
