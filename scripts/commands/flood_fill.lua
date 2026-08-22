function table.count(tab)
	local n = 0
	for k, v in pairs(tab) do
		n = n + 1
	end
	return n
end

local s_dirAxis = {
	{ forward = "y", right = "x", up = "z", forwardDir =  1, rightDir =  1, upDir =  1 }, -- Back
	{ forward = "z", right = "x", up = "y", forwardDir =  1, rightDir =  1, upDir = -1 }, -- Down
	{ forward = "y", right = "x", up = "z", forwardDir = -1, rightDir =  1, upDir = -1 }, -- Front
	{ forward = "z", right = "y", up = "x", forwardDir = -1, rightDir =  1, upDir = -1 }, -- Left
	{ forward = "z", right = "y", up = "x", forwardDir = -1, rightDir = -1, upDir =  1 }, -- Right
	{ forward = "z", right = "x", up = "y", forwardDir = -1, rightDir =  1, upDir =  1 }, -- Up
}

return function ()
	local env = CurrentPlayer:GetEnvironment()
	local physWorld = env:GetPhysWorld()

	local controller = CurrentPlayer:GetController()

	local eyePos = controller:GetEyePosition()
	local cameraRot = controller:GetCameraRotation()

	local result = physWorld:RaycastQueryFirst(eyePos, eyePos + cameraRot * Vec3(0, 0, -10), { IgnorePlayers = true })

	if not result.hitEntity or not result.hitChunk then
		print("no chunk hit")
		return
	end

	local chunkNode = result.hitEntity:GetComponent("node")
	local chunkRigidbody = result.hitEntity:GetComponent("rigidbody3d")

	local localPos = chunkNode:ToLocalPosition(result.hitPosition)
	local localRot = chunkNode:ToLocalDirection(result.hitNormal)

	local hitBlock = result.hitChunk:ComputeHitCoordinates(localPos, localRot, chunkRigidbody:GetCollider(), result.subShapeID)
	if not hitBlock then
		print("no block hit")
		return
	end

	local chunkContainer = result.hitChunk:GetContainer()
	local water = chunkContainer:GetBlockLibrary():GetBlockIndex("water")

	local blockIndices = chunkContainer:GetBlockIndices(result.hitChunk:GetIndices(), hitBlock.blockIndices)

	-- Get block on top of the one we hit
	local blockPos = chunkContainer:GetChunkOffset(result.hitChunk:GetIndices()) + result.hitChunk:GetBlockCenterPosition(hitBlock.blockIndices)
	local dirToCenter = (blockPos - chunkContainer:GetCenterPosition()):GetNormal()

	local dir = DirectionFromNormal(dirToCenter)
	local dirAxis = s_dirAxis[dir + 1]

	local function GetUpAxis(chunk, localIndices)
		local blockPos = chunkContainer:GetChunkOffset(chunk:GetIndices()) + chunk:GetBlockCenterPosition(localIndices)
		local dirToCenter = (blockPos - chunkContainer:GetCenterPosition()):GetNormal()

		local dir = DirectionFromNormal(dirToCenter)
		return s_dirAxis[dir + 1]
	end

	local dirAxis = GetUpAxis(result.hitChunk, hitBlock.blockIndices)
	blockIndices[dirAxis.up] = blockIndices[dirAxis.up] + dirAxis.upDir

	local counter = 1

	local pendingList = { blockIndices }

	local function AddNeighbor(chunkContainer, blockIndices, axis, dir, factor, checkAxis)
		local nextBlockIndices = Vec3(blockIndices.x, blockIndices.y, blockIndices.z)
		nextBlockIndices[axis[dir]] = nextBlockIndices[axis[dir]] + axis[dir .. "Dir"] * factor

		local nextChunkIndices, nextInnerCoordinates = chunkContainer:GetChunkIndicesByBlockIndices(nextBlockIndices)
		local nextChunk = chunkContainer:GetChunk(nextChunkIndices)
		if not nextChunk then
			-- out of bounds
			return
		end

		local nextAxis = GetUpAxis(nextChunk, nextInnerCoordinates)
		if checkAxis and axis ~= nextAxis then
			nextBlockIndices = Vec3(blockIndices.x, blockIndices.y, blockIndices.z)
			nextBlockIndices[nextAxis[dir]] = nextBlockIndices[nextAxis[dir]] + nextAxis[dir .. "Dir"] * factor
		end

		table.insert(pendingList, nextBlockIndices)
	end

	local updateCallback
	updateCallback = function ()
		local remaining = #pendingList
		if remaining == 0 then		
			return
		end

		Server.ScheduleForNextTick(updateCallback)

		local updatedChunks = {}

		for i = 1, remaining do
			local firstBlock = table.remove(pendingList, 1)

			local chunkIndices, innerCoordinates = chunkContainer:GetChunkIndicesByBlockIndices(firstBlock)
			local targetChunk = chunkContainer:GetChunk(chunkIndices)
			if not targetChunk then
				-- out of bounds
				return
			end

			if targetChunk:GetBlockContent(innerCoordinates) == 0 then
				local chunkKey = tostring(targetChunk:GetIndices())
				if not updatedChunks[chunkKey] then
					targetChunk:BeginBatchUpdate()
					updatedChunks[chunkKey] = targetChunk
				end

				targetChunk:UpdateBlock(innerCoordinates, water, true)

				-- add empty neighbor blocks
				local axis = GetUpAxis(targetChunk, innerCoordinates)

				AddNeighbor(chunkContainer, firstBlock, axis, "forward", 1, true)
				AddNeighbor(chunkContainer, firstBlock, axis, "forward", -1, true)
				AddNeighbor(chunkContainer, firstBlock, axis, "right", 1, true)
				AddNeighbor(chunkContainer, firstBlock, axis, "right", -1, true)
				AddNeighbor(chunkContainer, firstBlock, axis, "up", -1)
			end

			if table.count(updatedChunks) > 10 then
				for chunkKey, chunk in pairs(updatedChunks) do
					chunk:EndBatchUpdate()
				end
				-- limit concurrent chunk updates per tick
				return
			end
		end

		for chunkKey, chunk in pairs(updatedChunks) do
			chunk:EndBatchUpdate()
		end
	end

	Server.ScheduleForNextTick(updateCallback)
end