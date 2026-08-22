local modelSize = Vec3(0.2, 2.0, 1.0)
local offset = Vec3(0.0, 0.0, -0.4)

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "metal_door")
classData:Set("spawnable_collider", modelSize)
classData:Set("spawnable_offset", offset)

classData:AddProperty("opened", { type = "bool", default = false, isNetworked = true })

local collider = TranslatedRotatedCollider3D.new(BoxCollider3D.new(modelSize), offset)

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = collider,
		objectLayer = Constants.ObjectLayerStatic
	}

	local rigidBody = self:AddComponent("rigidbody3d", physSettings)
	self:SetInteractible(true)

	local node = self:GetComponent("node")
	self.AABB = rigidBody:GetAABB()
	self.InitialRotation = node:GetRotation()

	self:HandleOpened(self:GetProperty("opened"))

	if CLIENT then
		self:SetInteractibleText("Door")

		local model = AssetLibrary.GetModel("metal_door")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

function classData:HandleOpened(isOpen)
	local rotation = self.InitialRotation
	if isOpen then
		rotation = Quaternion.CombineRotations(rotation, EulerAngles(0, 90, 0):ToQuaternion())
	end

	self:GetComponent("rigidbody3d"):SetRotation(rotation)

	if SERVER then
		self:UpdateBlock(isOpen and "empty" or "area_blocker")
	else
		-- Change node only clientside (changing it serverside would change the saved rotation of the door)
		self:GetComponent("node"):SetRotation(rotation)
	end
end

if SERVER then
	classData:On("destroy", function (self)
		self:UpdateBlock("empty")
	end)

	function classData:UpdateBlock(blockName)
		-- TODO: Move this to C++
		local environment = self:GetEnvironment()
		local chunkContainer = environment:GetChunkContainer()
		local halfBlockSize = chunkContainer:GetTileSize() * 0.5

		local block = chunkContainer:GetBlockLibrary():GetBlockIndex(blockName)

		local maxPos = self.AABB:GetMaximum() - Vec3(halfBlockSize)
		local minPos = self.AABB:GetMinimum() + Vec3(halfBlockSize)

		local maxChunkIndices = chunkContainer:GetChunkIndicesByPosition(maxPos)
		local minChunkIndices = chunkContainer:GetChunkIndicesByPosition(minPos)

		local maxChunk = chunkContainer:GetChunk(maxChunkIndices)
		local minChunk = chunkContainer:GetChunk(minChunkIndices)

		local maxBlockIndices = chunkContainer:GetBlockIndices(maxChunkIndices, maxChunk:ComputeCoordinates(maxPos))
		local minBlockIndices = chunkContainer:GetBlockIndices(minChunkIndices, minChunk:ComputeCoordinates(minPos))

		for z = minBlockIndices.z, maxBlockIndices.z do
			for y = minBlockIndices.y, maxBlockIndices.y do
				for x = minBlockIndices.x, maxBlockIndices.x do
					local chunkIndices, localIndices = chunkContainer:GetChunkIndicesByBlockIndices(Vec3(x, y, z))
					local chunk = chunkContainer:GetChunk(chunkIndices)
					if chunk then
						chunk:UpdateBlock(localIndices, block)
					end
				end
			end
		end
	end

	classData:On("interact", function (self, player)
		local opened = not self:GetProperty("opened")
		self:UpdateProperty("opened", opened)

		self:HandleOpened(opened)
	end)
else
	classData:OnPropertyUpdate("opened", function (self, isOpen)
		self:HandleOpened(isOpen)
	end)
end

EntityRegistry.RegisterClass("metal_door", classData)
