local perlin = PerlinNoise()
local chunksize = 32
local scale = 0.2

return function (chunk, seed)
    perlin:reseed(seed)

    local blockLibrary = chunk:GetBlockLibrary()
    local blockCount = chunk:GetBlockCount()

    local empty = blockLibrary:GetBlockIndex("empty")
    local dirt = blockLibrary:GetBlockIndex("dirt")
    local grass = blockLibrary:GetBlockIndex("grass")

    local planet = chunk:GetContainer()
    local chunkIndices = chunk:GetIndices()

    local content = {}
    
    for z = 0, chunksize - 1 do
        for y = 0, chunksize - 1 do
            for x = 0, chunksize - 1 do
                local blockPos = planet:GetBlockIndices(chunkIndices, Vec3ui(x, y, z))
                local blockPosNorm, distToCenter = Vec3f(blockPos.x * 1.0, blockPos.y * 1.0, blockPos.z * 1.0):GetNormal()
                
                local height = perlin:normalizedOctave3D_01(blockPosNorm.x * scale, blockPosNorm.y * scale, blockPosNorm.z * scale, 4, 2.0)
                height = height * 100
                table.insert(content, distToCenter > height and empty or grass)
            end
        end
    end
    chunk:Reset(content)
end
