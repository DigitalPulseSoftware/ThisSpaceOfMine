-- Do not touch to this 2 variables
local perlin = PerlinNoise()
local chunksize = 32

local minGrenerationFreeHeight = 0 -- Generation height limit used to make generation faster if we want empty chunks to allow players to build tall things
local baseFreeHeight = 30 -- Should be greater than minFreeHeight, difference between both will define max generation height from baseFreeHeight

return function (chunk, seed)
    perlin:reseed(seed)

    local blockLibrary = chunk:GetBlockLibrary()
    local blockCount = chunk:GetBlockCount()

    local emptyBlock = blockLibrary:GetBlockIndex("empty")
    local debugBlock = blockLibrary:GetBlockIndex("debug")
    local dirtBlock = blockLibrary:GetBlockIndex("dirt")
    local grassBlock = blockLibrary:GetBlockIndex("grass")
    local hullBlock = blockLibrary:GetBlockIndex("hull")
    local hull2Block = blockLibrary:GetBlockIndex("hull2")
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
    
    local maxHeight = (chunksize * planet:GetChunkCount()^(1/3))/2;
    local maxGenerationHeight = maxHeight - minGrenerationFreeHeight
    local baseHeight = maxHeight - baseFreeHeight -- Only works for planets with the same number of chunks in all the directions
    
    local terrainVariation1Scale = 0.06 * baseHeight
    local terrainVariation2Scale = 0.16 * baseHeight
    local moutainScale = 0.03 * baseHeight
    local spikeScale = 0.2 * baseHeight
    local caveScale = 0.06 -- Other scale unit
    
    local content = {}
    
    for z = 0, chunksize - 1 do
        for y = 0, chunksize - 1 do
            for x = 0, chunksize - 1 do
                local blockPos = planet:GetBlockIndices(chunkIndices, Vec3ui(x, y, z))
                local blockPosNorm, distToCenter = Vec3f(blockPos.x * 1.0, blockPos.y * 1.0, blockPos.z * 1.0):GetNormal()
                distToCenter = math.max(math.abs(blockPos.x + 0.5), math.abs(blockPos.y + 0.5), math.abs(blockPos.z + 0.5))
                
                if distToCenter > maxGenerationHeight then
                    table.insert(content, emptyBlock)
                    goto continue
                end
                
                local blockPresence = perlin:normalizedOctave3D_01(blockPos.x * caveScale, blockPos.y * caveScale, blockPos.z * caveScale, 4, 0.1)
                
                if distToCenter <= baseHeight then
                    if blockPresence >= 0.3 and blockPresence <= 0.7 then
                        if distToCenter <= baseHeight-5 then
                            table.insert(content, stoneBlock)
                        elseif distToCenter <= baseHeight then
                            table.insert(content, dirtBlock)
                        end
                    else
                        table.insert(content, stoneBlock)
                    end
                else
                    local baseMountainous = perlin:normalizedOctave3D_01((blockPosNorm.x * moutainScale)+10, blockPosNorm.y * moutainScale, blockPosNorm.z * moutainScale, 4, 0.1)
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
                    if baseSpikeHeight < 0.7 then 
                        spikeHeight = 0
                    elseif baseSpikeHeight < 0.9 then 
                        spikeHeight = 5*baseSpikeHeight-3.5
                    else
                        spikeHeight = 1
                    end
                    spikeHeight = (1-mountainous) * spikeHeight * 20
                    
                    local height = baseHeight + heightVariation1 + heightVariation2 + spikeHeight
                    
                    if distToCenter <= height then
                        if distToCenter >= height - spikeHeight then
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
    
    chunk:Reset(content)
end
