return function (opt)
	local op = opt and opt.op
	assert(op, "missing op parameter")
	local inputSlot = opt and opt.inputSlot or 0
	local outputSlot = opt and opt.outputSlot or 0

	local env = CurrentPlayer:GetEnvironment()
	local physWorld = env:GetPhysWorld()

    local controller = CurrentPlayer:GetController()

	local eyePos = controller:GetEyePosition()
	local cameraRot = controller:GetCameraRotation()

	local result = physWorld:RaycastQueryFirst(eyePos, eyePos + cameraRot * Vec3(0, 0, -10), { IgnorePlayers = true })
	if not result.hitEntity then
		return
	end

	if op == "select1" then
		_G.FirstEntity = result.hitEntity
		_G.print("selected source entity")
	elseif op == "select2" then
		_G.SecondEntity = result.hitEntity
		print("selected destination entity")
	elseif op == "connect" then
		local sourceEntity = assert(_G.FirstEntity, "missing source entity")
		local destEntity = assert(_G.SecondEntity, "missing destination entity")
		local source = sourceEntity:GetComponent("distribution")
		local dest = destEntity:GetComponent("distribution")

		assert(inputSlot < dest:GetInputCount())
		assert(outputSlot < source:GetOutputCount())

		source:ConnectOutput(outputSlot, destEntity, inputSlot)
		dest:ConnectInput(inputSlot, sourceEntity, outputSlot)
		print(string.format("connected input #%d of source entity to output #%d of destination entity", inputSlot, outputSlot))
	else
		error("op must be select1, select2 or connect")
	end
end
