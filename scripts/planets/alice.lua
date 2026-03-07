local perlin = PerlinNoise()
local chunksize = 32
local scale = 0.02
local freespace = 30

return function (chunk, seed, chunkcount)
    perlin:reseed(seed)
    math.randomseed(seed)

    local blockLibrary = chunk:GetBlockLibrary()
    local blockCount = chunk:GetBlockCount()

    local empty = blockLibrary:GetBlockIndex("empty")
    local dirt = blockLibrary:GetBlockIndex("dirt")
    local grass = blockLibrary:GetBlockIndex("grass")
    local snow = blockLibrary:GetBlockIndex("snow")
    local stone = blockLibrary:GetBlockIndex("stone")
    local stoneMossy = blockLibrary:GetBlockIndex("stone_mossy")

    local planet = chunk:GetContainer()
    local chunkIndices = chunk:GetIndices()

    local maxHeight = (Vec3i(chunkcount.x, chunkcount.y, chunkcount.z) + Vec3i(1)) / 2
    maxHeight = maxHeight * chunksize

    local content = {}
    
    for z = 0, chunksize - 1 do
        for y = 0, chunksize - 1 do
            for x = 0, chunksize - 1 do
                local blockPos = planet:GetBlockIndices(chunkIndices, Vec3ui(x, y, z))
                local depth = math.min(
                    maxHeight.x - math.abs(blockPos.x),
                    maxHeight.y - math.abs(blockPos.y),
                    maxHeight.z - math.abs(blockPos.z)
                )

                if (depth < freespace) then
                    table.insert(content, empty)
                    goto continue
                end

                depth = depth - freespace
                
                local presence = perlin:normalizedOctave3D_01(blockPos.x * scale, blockPos.y * scale, blockPos.z * scale, 4, 0.5)
                if depth < 20 then
                    presence = presence * math.max(depth / 20.0, 1.0)
                end

                presence = presence + depth / math.max(maxHeight.x, maxHeight.y, maxHeight.z)

                local blockIndex
                if presence > 0.6 then
                    if depth < 6 * 2 then
                        blockIndex = snow
                    elseif depth <= 18 * 2 then
                        blockIndex = dirt
                    else
                        blockIndex = math.random() > 0.1 and stone or stoneMossy
                    end
                else
                    blockIndex = empty
                end

                table.insert(content, blockIndex)

                ::continue::
            end
        end
    end
    chunk:Reset(content)
end
