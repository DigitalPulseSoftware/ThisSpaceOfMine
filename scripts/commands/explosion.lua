local EmptyBlock = 0

local function Explode(physWorld, controller, radius)
	local eyePos = controller:GetEyePosition()
	local cameraRot = controller:GetCameraRotation()

	local result = physWorld:RaycastQueryFirst(eyePos, eyePos + cameraRot * Vec3(0, 0, -1000), { IgnorePlayers = true })
	if not result or not result.hitEntity or not result.hitChunk then
		return
	end

	local chunkNode = result.hitEntity:GetComponent("node")
	local chunkRigidbody = result.hitEntity:GetComponent("rigidbody3d")

	local localPos = chunkNode:ToLocalPosition(result.hitPosition)
	local localRot = chunkNode:ToLocalDirection(result.hitNormal)

	local hitBlock = result.hitChunk:ComputeHitCoordinates(localPos, localRot, chunkRigidbody:GetCollider(), result.subShapeID)
	if not hitBlock then
		return
	end

	local chunkContainer = result.hitChunk:GetContainer()

	local globalBlockIndices = chunkContainer:GetBlockIndices(result.hitChunk:GetIndices(), hitBlock.blockIndices)

    local updatedChunks = {}
    for z = -radius, radius do
        for y = -radius, radius do
            for x = -radius, radius do
                local pos = Vec3(x, y, z)
                if pos:GetLength() > radius then
                    goto continue
                end

                local chunkIndices, innerCoordinates = chunkContainer:GetChunkIndicesByBlockIndices(globalBlockIndices + Vec3(x, y, z))
                local targetChunk = chunkContainer:GetChunk(chunkIndices)
                if targetChunk then
                    targetChunk:UpdateBlock(innerCoordinates, EmptyBlock)
                    updatedChunks[tostring(targetChunk:GetIndices())] = true
                end

                ::continue::
            end
        end
    end
end

return function (opt)
    opt = opt or {}

    local radius = opt.radius or 5
    local interval = opt.interval or 0
    local duration = opt.duration or 5

	local env = CurrentPlayer:GetEnvironment()
	local physWorld = env:GetPhysWorld()

	local controller = CurrentPlayer:GetController()
    Explode(physWorld, controller, radius)

    local startTime = os.clock()
    local lastExplosion = startTime
    if interval > 0 then
        local tickFunction
        tickFunction = function ()
            local now = os.clock()

            if now - startTime > duration then
                return
            end

            Server.ScheduleForNextTick(tickFunction)

            if now - lastExplosion < interval then
                return
            end

            Explode(physWorld, controller, radius)
            lastExplosion = now
        end
        Server.ScheduleForNextTick(tickFunction)
    end
end