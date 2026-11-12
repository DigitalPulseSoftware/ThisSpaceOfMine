-- Do not touch to this 2 variables
local perlin = PerlinNoise()
local chunksize = 32

local function GetBlockIndices(chunkIndices, blockIndices)
    local x = chunkIndices.x * chunksize + blockIndices.x - chunksize * 0.5
    local y = chunkIndices.y * chunksize + blockIndices.z - chunksize * 0.5
    local z = chunkIndices.z * chunksize + blockIndices.y - chunksize * 0.5
    return Vec3(x, y, z)
end

return {
    PlanetType = "asteroid",
    Generator = function (chunk, chunkDims, properties)
        local radius = assert(properties.Radius, "missing Radius property")
        local seed = assert(properties.Seed, "missing Seed property")

        perlin:reseed(seed)

        local planet = chunk:GetContainer()
        local blockLibrary = planet:GetBlockLibrary()

        local blockSize = chunk:GetBlockSize()

        local emptyBlock = blockLibrary:GetBlockIndex("empty")
        local rockBlock = blockLibrary:GetBlockIndex("rock")

        local chunkIndices = chunk:GetIndices()

        local posOffset = Vec3(
            chunkDims.x % 2 == 0 and blockSize * chunksize * 0.5 or 0,
            chunkDims.y % 2 == 0 and blockSize * chunksize * 0.5 or 0,
            chunkDims.z % 2 == 0 and blockSize * chunksize * 0.5 or 0
        )

        local content = table.new(chunksize * chunksize * chunksize, 0)

        local noiseScale = 1.0

        for z = 0, chunksize - 1 do
            for y = 0, chunksize - 1 do
                for x = 0, chunksize - 1 do
                    local blockPos = GetBlockIndices(chunkIndices, Vec3(x, y, z))
                    local blockPosScaled = blockPos * blockSize + posOffset
                    local blockPosNorm, distToCenter = blockPosScaled:GetNormal()

                    local height = perlin:normalizedOctave3D(blockPosNorm.x * noiseScale, blockPosNorm.y * noiseScale, blockPosNorm.z * noiseScale, 4, 0.1)

                    local maxHeight = math.max(radius + height * radius, 1)

                    if distToCenter <= maxHeight then
                    --if z ~= 0 and y ~= 0 and x ~= 0 and z ~= chunksize -1 and y ~= chunksize -1 and x ~= chunksize -1 then
                        table.insert(content, rockBlock)
                    else
                        table.insert(content, emptyBlock)
                    end

                    ::continue::
                end
            end
        end

        return content
    end
}