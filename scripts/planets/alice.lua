-- Do not touch to this 2 variables
local perlin = PerlinNoise()
local chunksize = 32

local minGrenerationFreeHeight = 0 -- Generation height limit used to make generation faster if we want empty chunks to allow players to build tall things
local baseFreeHeight = 30 -- Should be greater than minFreeHeight, difference between both will define max generation height from baseFreeHeight

return function (chunk, seed, chunkDims)
    perlin:reseed(seed)

    local blockSize = chunk:GetBlockSize()

    local blockLibrary = chunk:GetBlockLibrary()
    local blockCount = chunk:GetBlockCount()

    local emptyBlock = blockLibrary:GetBlockIndex("empty")
    local debugBlock = blockLibrary:GetBlockIndex("debug")
    local dirtBlock = blockLibrary:GetBlockIndex("dirt")
    local grassBlock = blockLibrary:GetBlockIndex("grass")
    local hullBlock = blockLibrary:GetBlockIndex("hull")
    local snowBlock = blockLibrary:GetBlockIndex("snow")
    local stoneBlock = blockLibrary:GetBlockIndex("stone")
    local stoneMossyBlock = blockLibrary:GetBlockIndex("stone_mossy")
    local forcefieldBlock = blockLibrary:GetBlockIndex("forcefield")
    local planksBlock = blockLibrary:GetBlockIndex("planks")
    local stoneBricksBlock = blockLibrary:GetBlockIndex("stone_bricks")
    local copperBlock = blockLibrary:GetBlockIndex("copper_block")
    local glassBlock = blockLibrary:GetBlockIndex("glass")

    local planet = chunk:GetContainer()
    local chunkIndices = chunk:GetIndices()
    
    local maxHeight = (chunksize * chunkDims.x)/2 * blockSize;
    local maxGenerationHeight = maxHeight - minGrenerationFreeHeight
    local baseHeight = maxHeight - baseFreeHeight -- Only works for planets with the same number of chunks in all the directions
    
    local terrainVariation1Scale = 0.06 * baseHeight
    local terrainVariation2Scale = 0.16 * baseHeight
    local moutainScale = 0.035 * baseHeight
    local spikeScale = 0.2 * baseHeight
    local caveScale = 0.06  -- Other scale unit
    
    local content = {}
    
    for z = 0, chunksize - 1 do
        for y = 0, chunksize - 1 do
            for x = 0, chunksize - 1 do
                local blockPos = planet:GetBlockIndices(chunkIndices, Vec3ui(x, y, z))
                local blockPosScaled = Vec3f(blockPos.x * 0.5, blockPos.y * 0.5, blockPos.z * 0.5)
                local blockPosNorm, distToCenter = blockPosScaled:GetNormal()
                --distToCenter = math.max(math.abs(blockPos.x * 0.5 + 0.5), math.abs(blockPos.y * 0.5 + 0.5), math.abs(blockPos.z * 0.5 + 0.5))
                distToCenter = SignedDistance.RoundBox(blockPosScaled, Vec3f(baseHeight), 16.0)

                if distToCenter > baseFreeHeight then
                    table.insert(content, emptyBlock)
                    goto continue
                end

                local blockPresence = perlin:normalizedOctave3D_01(blockPosScaled.x * caveScale, blockPosScaled.y * caveScale, blockPosScaled.z * caveScale, 4, 0.1)
                
                if distToCenter <= -32.0 then
                    if blockPresence >= 0.3 and blockPresence <= 0.7 then
                        if distToCenter <= -5 then
                            table.insert(content, stoneBlock)
                        else
                            table.insert(content, dirtBlock)
                        end
                    else
                        table.insert(content, stoneBlock)
                    end
                else
                    local baseMountainous = perlin:normalizedOctave3D_01((blockPosNorm.x * moutainScale)+10, blockPosNorm.y * moutainScale, blockPosNorm.z * moutainScale, 4, 0.1)
                    local mountainous
                    if baseMountainous < 0.6 then 
                        mountainous = 0
                    elseif baseMountainous < 0.8 then 
                        mountainous = 5*baseMountainous-3
                    else
                        mountainous = 1
                    end
                    
                    local heightVariation1 = 10 * perlin:normalizedOctave3D_01(blockPosNorm.x * terrainVariation1Scale, blockPosNorm.y * terrainVariation1Scale, blockPosNorm.z * terrainVariation1Scale, 4, 0.1)
                    local heightVariation2 = 40 * mountainous * perlin:normalizedOctave3D_01((blockPosNorm.x * terrainVariation2Scale)+20, blockPosNorm.y * terrainVariation2Scale, blockPosNorm.z * terrainVariation2Scale, 4, 0.1)
                    
                    local baseSpikeHeight = perlin:normalizedOctave3D_01((blockPosNorm.x * spikeScale)+30, blockPosNorm.y * spikeScale, blockPosNorm.z * spikeScale, 4, 0.1)
                    
                    local height = heightVariation1 + heightVariation2
                    
                    if distToCenter <= height then
                        if distToCenter >= height then
                            table.insert(content, stoneMossyBlock)
                        elseif mountainous > 0.5 and heightVariation2 > 0.5 then
                            table.insert(content, snowBlock)
                        elseif mountainous > 0.1 then
                            table.insert(content, stoneBlock)
                        elseif baseMountainous < 0.4 then
                            table.insert(content, grassBlock)
                        else
                            table.insert(content, dirtBlock)
                        end
                    else
                        table.insert(content, emptyBlock)
                    end
                end
                
                ::continue::
            end
        end
    end

    return content
end
